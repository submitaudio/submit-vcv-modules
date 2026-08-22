#include "../src/FlipEngine.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

static void testGridClockDerivesMusicalSubdivisions() {
	FlipGridClock clock;
	clock.reset();
	std::vector<int> boundaries;

	// The first 1-PPQN edge establishes phase only. Once the eight-sample
	// interval is measured, division 4 produces exact positions 8, 10, 12, 14, 16.
	for (int sample = 0; sample <= 16; ++sample) {
		const bool rawClock = sample == 0 || sample == 8 || sample == 16;
		if (clock.process(rawClock, 4))
			boundaries.push_back(sample);
	}

	const std::vector<int> expected = {8, 10, 12, 14, 16};
	assert(boundaries == expected);
}

static void testLengthIsIgnoredWithoutFreeze() {
	FlipGridClock clock;
	FlipEngine engine;
	clock.reset();
	engine.setCapacity(128);
	engine.arm();

	std::vector<std::uint64_t> scheduledDurations;
	for (int sample = 0; sample <= 24; ++sample) {
		const bool rawClock = sample % 8 == 0;
		// The UI may request 1/8, but live (non-frozen) mode must force one
		// complete quarter-note cell.
		const bool boundary = clock.process(rawClock, flipGridDivision(false, 2));
		const FlipEngine::Frame frame = engine.process(
			static_cast<float>(sample), 0.f, boundary, true, false);
		// The boundary at sample 8 closes the complete live cell.
		if (sample > 8 && frame.timing.valid)
			scheduledDurations.push_back(frame.timing.effectivePlaybackDuration);
	}

	// Normal (non-frozen) playback remains one full clock cycle regardless of
	// the Length control.
	assert(!scheduledDurations.empty());
	for (std::uint64_t duration : scheduledDurations)
		assert(duration == 8);
}

static void testLengthBecomesActiveOnlyWhenFrozen() {
	FlipGridClock clock;
	clock.reset();
	std::vector<int> boundaries;

	for (int sample = 0; sample <= 16; ++sample) {
		const bool rawClock = sample == 0 || sample == 8 || sample == 16;
		const bool freeze = sample >= 11;
		const int division = flipGridDivision(freeze, 2);
		if (clock.process(rawClock, division))
			boundaries.push_back(sample);
	}

	// Freeze is enabled at sample 11. Grid point 10 is already past, so the
	// first valid 1/8 point is 12; live mode before that stayed at quarter-note
	// boundaries.
	const std::vector<int> expected = {0, 8, 12, 13, 14, 15, 16};
	assert(boundaries == expected);
}

static void testWaitsForGateAndClock() {
	FlipEngine engine;
	engine.setCapacity(32);

	assert(engine.process(1.f, -1.f, true).state == FlipEngine::State::IDLE);
	engine.arm();
	assert(engine.process(2.f, -2.f, false).state == FlipEngine::State::ARMED);
	assert(engine.process(3.f, -3.f, true).state == FlipEngine::State::RECORDING);
}

static void testExactReverseCycle() {
	FlipEngine engine;
	engine.setCapacity(32);
	engine.arm();

	engine.process(10.f, -10.f, true);
	engine.process(11.f, -11.f, false);
	engine.process(12.f, -12.f, false);

	// Capture region [10, 11, 12] is read backwards without rotation.
	FlipEngine::Frame frame = engine.process(99.f, -99.f, true);
	assert(frame.outputLeft == 12.f && frame.outputRight == -12.f);
	frame = engine.process(99.f, -99.f, false);
	assert(frame.outputLeft == 11.f && frame.outputRight == -11.f);
	frame = engine.process(99.f, -99.f, false);
	assert(frame.outputLeft == 10.f && frame.outputRight == -10.f);
	assert(frame.playbackSample);
	assert(frame.state == FlipEngine::State::IDLE);
}

static void testContinuousDoubleBuffering() {
	FlipEngine engine;
	engine.setCapacity(32);
	engine.arm();

	engine.process(1.f, -1.f, true, true);
	engine.process(2.f, -2.f, false, true);
	engine.process(3.f, -3.f, false, true);
	FlipEngine::Frame frame = engine.process(4.f, -4.f, true, true);
	assert(frame.outputLeft == 3.f && frame.outputRight == -3.f);
	assert(engine.process(5.f, -5.f, false, true).outputLeft == 2.f);
	assert(engine.process(6.f, -6.f, false, true).outputLeft == 1.f);

	frame = engine.process(7.f, -7.f, true, true);
	assert(frame.outputLeft == 6.f && frame.outputRight == -6.f);
	assert(frame.state == FlipEngine::State::PLAYING);
}

static void testReverseEndpointFinishesOnTargetBeat() {
	FlipEngine engine;
	engine.setCapacity(64);
	engine.arm();

	constexpr std::uint64_t period = 8;
	constexpr std::uint64_t simulationLength = 40;
	int timingEvents = 0;
	int endpointEvents = 0;

	for (std::uint64_t sample = 0; sample < simulationLength; ++sample) {
		const bool clockEdge = sample % period == 0;
		const float input = clockEdge ? 10.f : 0.f;
		const FlipEngine::Frame frame = engine.process(input, input, clockEdge, true);

		if (frame.timing.valid) {
			++timingEvents;
			std::cout
				<< "targetBeatTime=" << frame.timing.targetBeatTime
				<< " reverseStartTime=" << frame.timing.reverseStartTime
				<< " effectivePlaybackDuration=" << frame.timing.effectivePlaybackDuration
				<< " actualAudioStartTime=" << frame.timing.actualAudioStartTime
				<< " actualAudioEndTime=" << frame.timing.actualAudioEndTime
				<< " audioBufferLatency=" << frame.timing.audioBufferLatency
				<< '\n';

			assert(frame.timing.reverseStartTime
				== frame.timing.targetBeatTime - frame.timing.effectivePlaybackDuration);
			assert(frame.timing.actualAudioStartTime == frame.timing.reverseStartTime);
			assert(frame.timing.actualAudioEndTime == frame.timing.targetBeatTime);
			assert(frame.timing.effectivePlaybackDuration == period);
			assert(frame.timing.audioBufferLatency == 0);
		}

		// The original grid transient is the final reversed sample. It occupies
		// [targetBeatTime - 1, targetBeatTime), so its endpoint is the beat.
		if (frame.playbackSample && frame.outputLeft == 10.f) {
			++endpointEvents;
			assert((sample + 1) % period == 0);
		}
	}

	assert(timingEvents == 4);
	assert(endpointEvents == 4);
}

static void testBoundaryDeClickDoesNotMoveEndpoint() {
	FlipEngine engine;
	engine.setCapacity(256);
	engine.setDeClickSamples(8);
	engine.arm();

	constexpr int period = 64;
	// First captured cell: its first sample (5) must remain the endpoint.
	for (int sample = 0; sample < period; ++sample)
		engine.process(sample == 0 ? 5.f : 0.f, 0.f, sample == 0, true);

	// Second cell starts at 7 and otherwise stays at -5. While it is captured, the first
	// cell plays backwards and finishes with 5 immediately before the boundary.
	for (int sample = 0; sample < period; ++sample) {
		const float input = sample == 0 ? 7.f : -5.f;
		const FlipEngine::Frame frame = engine.process(input, 0.f, sample == 0, true);
		if (sample == period - 1)
			assert(frame.outputLeft == 5.f);
	}

	// Raw playback would jump from +5 to -5 here. The correction removes that
	// discontinuity without delaying or rotating the new reverse segment.
	FlipEngine::Frame frame = engine.process(0.f, 0.f, true, true);
	assert(frame.outputLeft == 5.f);
	assert(frame.timing.reverseStartTime == 2 * period);
	assert(frame.timing.targetBeatTime == 3 * period);

	float previous = frame.outputLeft;
	float largestStep = 0.f;
	for (int sample = 1; sample < period; ++sample) {
		frame = engine.process(0.f, 0.f, false, true);
		if (sample <= 8)
			largestStep = std::max(largestStep, std::fabs(frame.outputLeft - previous));
		previous = frame.outputLeft;
	}
	assert(largestStep < 3.f);
	// The correction is long finished here; the original start sample is still
	// the unmodified endpoint immediately before the target beat.
	assert(frame.outputLeft == 7.f);
}

static void testFractionalClockRoundingNeverInsertsSilence() {
	FlipEngine engine;
	engine.setCapacity(128);
	engine.arm();

	// Fractional sample periods commonly alternate N/N+1. The old scheduler
	// emitted one zero sample at 16 and 33 because the previous capture was one
	// sample shorter than the current clock cell.
	for (int sample = 0; sample < 34; ++sample) {
		const bool clockEdge = sample == 0 || sample == 8 || sample == 17 || sample == 25;
		const FlipEngine::Frame frame = engine.process(1.f, 1.f, clockEdge, true);
		if (sample >= 8) {
			assert(frame.playbackSample);
			assert(frame.outputLeft == 1.f);
		}
	}
}

static void testFreezeRepeatsCapturedCellAndReleaseReturnsLive() {
	FlipEngine engine;
	engine.setCapacity(64);
	engine.arm();

	// Capture A = [1, 2, 3].
	engine.process(1.f, -1.f, true, true, false);
	engine.process(2.f, -2.f, false, true, false);
	engine.process(3.f, -3.f, false, true, false);

	// Play A while capturing a live replacement B.
	assert(engine.process(10.f, -10.f, true, true, false).outputLeft == 3.f);
	assert(engine.process(11.f, -11.f, false, true, false).outputLeft == 2.f);
	assert(engine.process(12.f, -12.f, false, true, false).outputLeft == 1.f);

	// Freeze latches the freshly completed replacement B, not the older bank A.
	// This keeps the frozen material on the same musical cell as live reverse.
	assert(engine.process(20.f, -20.f, true, true, true).outputLeft == 12.f);
	assert(engine.process(21.f, -21.f, false, true, true).outputLeft == 11.f);
	assert(engine.process(22.f, -22.f, false, true, true).outputLeft == 10.f);
	assert(engine.process(30.f, -30.f, true, true, true).outputLeft == 12.f);
	assert(engine.process(31.f, -31.f, false, true, true).outputLeft == 11.f);
	assert(engine.process(32.f, -32.f, false, true, true).outputLeft == 10.f);

	// Release starts a fresh full-cycle live capture. It must not replay the
	// stale partial data accumulated before Freeze was engaged.
	const FlipEngine::Frame released = engine.process(40.f, -40.f, true, true, false);
	assert(!released.timing.valid);
	engine.process(41.f, -41.f, false, true, false);
	engine.process(42.f, -42.f, false, true, false);
	const FlipEngine::Frame live = engine.process(43.f, -43.f, true, true, false);
	assert(live.outputLeft == 42.f && live.outputRight == -42.f);
	assert(live.timing.valid);
}

static void testFreezePreservesShortKickPeak() {
	FlipEngine engine;
	engine.setCapacity(128);
	engine.setDeClickSamples(8);
	engine.arm();

	constexpr int period = 16;
	// A short kick at the start of the captured cell. The reversed peak must
	// remain sample-accurate at the cell endpoint when Freeze restarts it.
	for (int sample = 0; sample < period; ++sample) {
		const float kick = sample == 0 ? 1.f
			: (sample == 1 ? 0.6f : (sample == 2 ? 0.25f : 0.f));
		engine.process(kick, kick, sample == 0, true, false);
	}

	// Start a live replacement cell with the same clock-aligned kick while A
	// plays backwards. Freeze must latch this freshly completed cell.
	for (int sample = 0; sample < period; ++sample) {
		const float kick = sample == 0 ? 1.f
			: (sample == 1 ? 0.6f : (sample == 2 ? 0.25f : 0.f));
		engine.process(kick, kick, sample == 0, true, false);
	}

	// Freeze restarts A. Its short transient must not be attenuated by the
	// boundary de-click correction.
	float peak = 0.f;
	for (int sample = 0; sample < period; ++sample) {
		const FlipEngine::Frame frame = engine.process(
			0.f, 0.f, sample == 0, true, true);
		if (frame.playbackSample)
			peak = std::max(peak, std::fabs(frame.outputLeft));
	}
	assert(peak == 1.f);
}

static void testFrozenEighthLengthProducesAudio() {
	FlipGridClock clock;
	FlipEngine engine;
	clock.reset();
	engine.setCapacity(256);
	engine.arm();

	constexpr int period = 32;
	int playbackSamples = 0;
	float peak = 0.f;
	for (int sample = 0; sample < period * 4; ++sample) {
		const bool clockEdge = sample % period == 0;
		// A short kick aligned to the first quarter-note edge.
		const int phase = sample % period;
		const float input = phase == 0 ? 1.f : (phase == 1 ? 0.5f : 0.f);
		const bool boundary = clock.process(clockEdge, flipGridDivision(true, 2));
		const FlipEngine::Frame frame = engine.process(input, input, boundary, true, true);
		if (frame.playbackSample) {
			++playbackSamples;
			peak = std::max(peak, std::fabs(frame.outputLeft));
		}
	}

	assert(playbackSamples > 0);
	assert(peak == 1.f);
}

static void testFrozenBufferReslicesOnLengthChange() {
	FlipEngine engine;
	engine.setCapacity(64);
	engine.arm();

	// Capture a full cell with the kick attack at its first sample.
	engine.process(1.f, 1.f, true, true, false);
	for (int sample = 1; sample < 8; ++sample)
		engine.process(0.f, 0.f, false, true, false);

	// Freeze at the capture boundary. The completed eight-sample buffer is
	// latched and reduced to two samples around the measured peak.
	engine.process(0.f, 0.f, true, true, true, 2);
	const FlipEngine::Frame endpoint =
		engine.process(0.f, 0.f, false, true, true, 2);
	assert(endpoint.outputLeft == 1.f && endpoint.outputRight == 1.f);
}

static void testFrozenLengthCanExpandAfterShorterWindows() {
	FlipEngine engine;
	engine.setCapacity(64);
	engine.arm();

	for (int sample = 0; sample < 8; ++sample)
		engine.process(static_cast<float>(sample + 1), 0.f, sample == 0, true, false);

	FlipEngine::Frame frame = engine.process(0.f, 0.f, true, true, true, 8);
	assert(frame.timing.effectivePlaybackDuration == 8);
	frame = engine.process(0.f, 0.f, true, true, true, 4);
	assert(frame.timing.effectivePlaybackDuration == 4);
	frame = engine.process(0.f, 0.f, true, true, true, 2);
	assert(frame.timing.effectivePlaybackDuration == 2);
	frame = engine.process(0.f, 0.f, true, true, true, 8);
	assert(frame.timing.effectivePlaybackDuration == 8);
}

static void testOverflowReturnsToIdle() {
	FlipEngine engine;
	engine.setCapacity(2);
	engine.arm();
	engine.process(1.f, -1.f, true);
	engine.process(2.f, -2.f, false);
	const FlipEngine::Frame overflow = engine.process(3.f, -3.f, false);
	assert(overflow.state == FlipEngine::State::IDLE);
	assert(overflow.captureOverflow);
	assert(!overflow.playbackSample);
}

int main() {
	testGridClockDerivesMusicalSubdivisions();
	testLengthIsIgnoredWithoutFreeze();
	testLengthBecomesActiveOnlyWhenFrozen();
	testWaitsForGateAndClock();
	testExactReverseCycle();
	testContinuousDoubleBuffering();
	testReverseEndpointFinishesOnTargetBeat();
	testBoundaryDeClickDoesNotMoveEndpoint();
	testFractionalClockRoundingNeverInsertsSilence();
	testFreezeRepeatsCapturedCellAndReleaseReturnsLive();
	testFreezePreservesShortKickPeak();
	testFrozenEighthLengthProducesAudio();
	testFrozenBufferReslicesOnLengthChange();
	testFrozenLengthCanExpandAfterShorterWindows();
	testOverflowReturnsToIdle();
}
