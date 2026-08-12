#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <vector>

/**
 * Length is a Freeze-only control. In live mode Flip always follows the full
 * incoming quarter-note clock cycle, so changing Length cannot shorten or
 * retime a kick (or any other live input).
 */
inline int flipGridDivision(bool freezeActive, int lengthIndex) {
	if (!freezeActive)
		return 1;
	if (lengthIndex <= 0)
		return 2;
	if (lengthIndex == 1)
		return 4;
	return 8;
}

/** Derives sample-accurate musical subdivisions from a 1-PPQN clock. */
class FlipGridClock {
public:
	void reset() {
		samplesSinceClock = 0;
		clockPeriod = 0;
		activeDivision = 1;
		nextSubdivision = 1;
		haveClock = false;
	}

	bool process(bool clockEdge, int requestedDivision) {
		requestedDivision = std::max(1, requestedDivision);
		bool boundary = false;
		if (clockEdge) {
			// The first edge establishes the phase, but cannot yet establish a
			// subdivision length. For a frozen 1/2, 1/4 or 1/8 cell, do not start a
			// capture on that unmeasured edge; otherwise the first full cycle is
			// restarted before its short reverse cell can reach its endpoint.
			const bool measuredClockPeriod = haveClock && samplesSinceClock > 0;
			if (measuredClockPeriod)
				clockPeriod = samplesSinceClock;
			haveClock = true;
			samplesSinceClock = 0;
			activeDivision = requestedDivision;
			nextSubdivision = 1;
			boundary = requestedDivision == 1 || measuredClockPeriod;
		}
		else if (haveClock && clockPeriod > 0) {
			if (requestedDivision != activeDivision) {
				activeDivision = requestedDivision;
				nextSubdivision = 1;
				// Skip subdivision points that are already in the past. The new
				// setting becomes active at the next valid point in this beat.
				while (nextSubdivision < activeDivision) {
					const std::uint64_t passedTarget =
						(clockPeriod * static_cast<std::uint64_t>(nextSubdivision)
							+ static_cast<std::uint64_t>(activeDivision / 2))
						/ static_cast<std::uint64_t>(activeDivision);
					if (passedTarget > samplesSinceClock)
						break;
					++nextSubdivision;
				}
			}

			if (nextSubdivision < activeDivision) {
				const std::uint64_t target =
					(clockPeriod * static_cast<std::uint64_t>(nextSubdivision)
						+ static_cast<std::uint64_t>(activeDivision / 2))
					/ static_cast<std::uint64_t>(activeDivision);
				if (samplesSinceClock >= target) {
					boundary = true;
					++nextSubdivision;
				}
			}
		}

		++samplesSinceClock;
		return boundary;
	}

	std::size_t getCellSamples() const {
		if (!haveClock || clockPeriod == 0)
			return 0;
		const std::uint64_t rounded =
			(clockPeriod + static_cast<std::uint64_t>(activeDivision / 2))
			/ static_cast<std::uint64_t>(activeDivision);
		return static_cast<std::size_t>(std::max<std::uint64_t>(1, rounded));
	}

private:
	std::uint64_t samplesSinceClock = 0;
	std::uint64_t clockPeriod = 0;
	int activeDivision = 1;
	int nextSubdivision = 1;
	bool haveClock = false;
};

/** Rack-independent, clock-synchronous stereo reverse engine. */
class FlipEngine {
public:
	enum class State {
		IDLE,
		ARMED,
		RECORDING,
		PLAYING
	};

	struct Frame {
		struct TimingLog {
			bool valid = false;
			std::uint64_t targetBeatTime = 0;
			std::uint64_t reverseStartTime = 0;
			std::uint64_t effectivePlaybackDuration = 0;
			std::uint64_t actualAudioStartTime = 0;
			std::uint64_t actualAudioEndTime = 0;
			std::uint64_t audioBufferLatency = 0;
		};

		float outputLeft;
		float outputRight;
		State state;
		bool captureOverflow;
		bool playbackSample;
		TimingLog timing;

		Frame(float outputLeft, float outputRight, State state, bool captureOverflow,
			bool playbackSample, const TimingLog& timing)
			: outputLeft(outputLeft), outputRight(outputRight), state(state),
			  captureOverflow(captureOverflow), playbackSample(playbackSample), timing(timing) {
		}
	};

	void setCapacity(std::size_t samples) {
		for (int bank = 0; bank < 2; ++bank) {
			bufferLeft[bank].assign(samples, 0.f);
			bufferRight[bank].assign(samples, 0.f);
		}
		reset();
	}

	void setDeClickSamples(std::size_t samples) {
		deClickSamples = samples;
	}

	void reset() {
		state = State::IDLE;
		captureBank = 0;
		captureSamples = 0;
		playbackBank = 0;
		playbackLength = 0;
		playbackSourceLength = 0;
		playbackStartOffset = 0;
		playbackPosition = 0;
		captureOverflow = false;
		sampleTime = 0;
		havePlaybackOutput = false;
		boundaryCorrectionPending = false;
		boundaryCorrectionRemaining = 0;
		boundaryCorrectionTotal = 0;
		boundaryCorrectionLeft = 0.f;
		boundaryCorrectionRight = 0.f;
		lastOutputLeft = 0.f;
		lastOutputRight = 0.f;
		endpointHoldSamples = 0;
		previousFreeze = false;
		releaseCapturePending = false;
	}

	void arm() {
		if (state == State::IDLE && !bufferLeft[0].empty()) {
			state = State::ARMED;
			captureOverflow = false;
		}
	}

	State getState() const {
		return state;
	}

	std::size_t getRecordedSamples() const {
		return captureSamples;
	}

	Frame process(float inputLeft, float inputRight, bool clockEdge, bool repeat = false,
		bool freeze = false, std::size_t frozenCellSamples = 0) {
		float outputLeft = 0.f;
		float outputRight = 0.f;
		bool playbackSample = false;
		Frame::TimingLog timing;

		// Freeze owns the current playback bank. When it is released, discard
		// the partial background capture and wait for a new full live cycle.
		const bool freezeActivated = !previousFreeze && freeze;
		if (previousFreeze && !freeze) {
			releaseCapturePending = true;
			captureSamples = 0;
		}
		previousFreeze = freeze;

		switch (state) {
			case State::IDLE:
				break;

			case State::ARMED:
				if (clockEdge) {
					beginCapture(0, inputLeft, inputRight);
					state = State::RECORDING;
				}
				break;

			case State::RECORDING:
				if (clockEdge) {
					timing = beginPlaybackFromCapture(freeze ? frozenCellSamples : 0);
					if (repeat) {
						if (freeze) {
							// Keep the just-captured playback bank fixed while frozen.
							captureSamples = 0;
						}
						else {
							beginCapture(1 - playbackBank, inputLeft, inputRight);
							releaseCapturePending = false;
						}
					}
					state = State::PLAYING;
					playbackSample = renderPlayback(outputLeft, outputRight);
				}
				else if (!freeze && !releaseCapturePending) {
					appendCapture(inputLeft, inputRight);
				}
				break;

			case State::PLAYING:
				if (repeat && clockEdge) {
					if (freeze) {
						if (freezeActivated && captureSamples > 0) {
							// Latch the cell that has just finished capturing. The live
							// engine switches to this same musical cell on this boundary;
							// restarting the older playback bank would put Freeze one cell
							// behind and can make patterned material sound offbeat.
							timing = beginPlaybackFromCapture(frozenCellSamples);
						}
						else {
							// Once latched, repeat this exact bank without overwriting it.
							configureFrozenWindow(frozenCellSamples);
							timing = restartPlayback();
						}
						captureSamples = 0;
						releaseCapturePending = false;
					}
					else if (releaseCapturePending || captureSamples == 0) {
						// Start a fresh full-cycle live capture after releasing Freeze.
						beginCapture(1 - playbackBank, inputLeft, inputRight);
						releaseCapturePending = false;
					}
					else {
						timing = beginPlaybackFromCapture();
						beginCapture(1 - playbackBank, inputLeft, inputRight);
					}
				}
				else if (repeat && !clockEdge && !freeze && !releaseCapturePending) {
					appendCapture(inputLeft, inputRight);
				}

				playbackSample = renderPlayback(outputLeft, outputRight);
				if (repeat && !playbackSample && havePlaybackOutput
					&& endpointHoldSamples < MAX_CLOCK_ROUNDING_SAMPLES) {
					// A fractional BPM can make clock cells alternate between N and
					// N+1 samples. Never insert a zero-valued gap while waiting for
					// that rounding sample; keep the true reversed endpoint instead.
					outputLeft = lastOutputLeft;
					outputRight = lastOutputRight;
					playbackSample = true;
					++endpointHoldSamples;
				}
				if (!repeat && playbackPosition >= playbackLength)
					state = State::IDLE;
				break;
		}

		++sampleTime;
		return {outputLeft, outputRight, state, captureOverflow, playbackSample, timing};
	}

private:
	std::vector<float> bufferLeft[2];
	std::vector<float> bufferRight[2];
	State state = State::IDLE;
	int captureBank = 0;
	std::size_t captureSamples = 0;
	int playbackBank = 0;
	std::size_t playbackLength = 0;
	std::size_t playbackSourceLength = 0;
	std::size_t playbackStartOffset = 0;
	std::size_t playbackPosition = 0;
	bool captureOverflow = false;
	std::uint64_t sampleTime = 0;
	std::size_t deClickSamples = 0;
	std::size_t boundaryCorrectionRemaining = 0;
	std::size_t boundaryCorrectionTotal = 0;
	float boundaryCorrectionLeft = 0.f;
	float boundaryCorrectionRight = 0.f;
	float lastOutputLeft = 0.f;
	float lastOutputRight = 0.f;
	bool havePlaybackOutput = false;
	bool boundaryCorrectionPending = false;
	std::size_t endpointHoldSamples = 0;
	bool previousFreeze = false;
	bool releaseCapturePending = false;
	static constexpr std::size_t MAX_CLOCK_ROUNDING_SAMPLES = 2;

	void beginCapture(int bank, float left, float right) {
		captureBank = bank;
		captureSamples = 1;
		bufferLeft[captureBank][0] = left;
		bufferRight[captureBank][0] = right;
	}

	void appendCapture(float left, float right) {
		if (captureSamples >= bufferLeft[captureBank].size()) {
			captureOverflow = true;
			state = State::IDLE;
			captureSamples = 0;
			return;
		}
		bufferLeft[captureBank][captureSamples] = left;
		bufferRight[captureBank][captureSamples] = right;
		++captureSamples;
	}

	Frame::TimingLog beginPlaybackFromCapture(std::size_t requestedLength = 0) {
		playbackBank = captureBank;
		playbackSourceLength = captureSamples;
		playbackLength = playbackSourceLength;
		playbackStartOffset = 0;
		if (requestedLength > 0)
			configureFrozenWindow(requestedLength);
		return restartPlayback();
	}

	void configureFrozenWindow(std::size_t requestedLength) {
		if (playbackSourceLength == 0 || requestedLength == 0)
			return;

		const std::size_t windowLength =
			std::min(requestedLength, playbackSourceLength);
		if (windowLength == playbackLength && playbackStartOffset < playbackSourceLength)
			return;

		playbackLength = windowLength;
		if (windowLength >= playbackSourceLength) {
			playbackStartOffset = 0;
			return;
		}

		// Put the strongest captured stereo transient at the end of the
		// reversed window, so a frozen kick resolves on the selected grid beat.
		std::size_t peakIndex = 0;
		float peak = 0.f;
		for (std::size_t i = 0; i < playbackSourceLength; ++i) {
			const float magnitude = std::max(
				std::fabs(bufferLeft[playbackBank][i]),
				std::fabs(bufferRight[playbackBank][i]));
			if (magnitude > peak) {
				peak = magnitude;
				peakIndex = i;
			}
		}
		playbackStartOffset = std::min(peakIndex, playbackSourceLength - windowLength);
	}

	Frame::TimingLog restartPlayback() {
		playbackPosition = 0;
		endpointHoldSamples = 0;
		boundaryCorrectionPending = havePlaybackOutput && deClickSamples > 0;

		// Minimal scheduler: the captured region is [0, playbackLength), rate is
		// exactly 1.0, and there is no envelope, interpolation or phase rotation.
		// The final reversed sample occupies [targetBeatTime - 1, targetBeatTime),
		// so the audio finishes exactly on the target grid boundary.
		Frame::TimingLog timing;
		timing.valid = playbackLength > 0;
		timing.effectivePlaybackDuration = playbackLength;
		timing.reverseStartTime = sampleTime;
		timing.targetBeatTime = timing.reverseStartTime + timing.effectivePlaybackDuration;
		timing.actualAudioStartTime = sampleTime;
		timing.actualAudioEndTime = sampleTime + playbackLength;
		timing.audioBufferLatency = 0;
		return timing;
	}

	bool renderPlayback(float& left, float& right) {
		if (playbackPosition >= playbackLength || playbackLength == 0)
			return false;

		const std::size_t readIndex =
			playbackStartOffset + playbackLength - 1 - playbackPosition;
		const float rawLeft = bufferLeft[playbackBank][readIndex];
		const float rawRight = bufferRight[playbackBank][readIndex];

		// Remove only the instantaneous value jump at a bank boundary. This is
		// an amplitude correction on the new segment, not a read-position shift:
		// playback still starts on the same sample and finishes on the same beat.
		if (boundaryCorrectionPending) {
			boundaryCorrectionTotal = playbackLength >= 8
				? std::min(deClickSamples, playbackLength / 8) : 0;
			if (boundaryCorrectionTotal < 2)
				boundaryCorrectionTotal = 0;
			boundaryCorrectionRemaining = boundaryCorrectionTotal;
			boundaryCorrectionLeft = lastOutputLeft - rawLeft;
			boundaryCorrectionRight = lastOutputRight - rawRight;
			boundaryCorrectionPending = false;
		}

		if (boundaryCorrectionRemaining > 0) {
			const std::size_t correctionPosition =
				boundaryCorrectionTotal - boundaryCorrectionRemaining;
			const float phase = static_cast<float>(correctionPosition)
				/ static_cast<float>(boundaryCorrectionTotal - 1);
			const float smoothPhase = phase * phase * (3.f - 2.f * phase);
			const float correctionGain = 1.f - smoothPhase;
			left = rawLeft + boundaryCorrectionLeft * correctionGain;
			right = rawRight + boundaryCorrectionRight * correctionGain;
			--boundaryCorrectionRemaining;
		}
		else {
			left = rawLeft;
			right = rawRight;
		}

		lastOutputLeft = left;
		lastOutputRight = right;
		havePlaybackOutput = true;
		++playbackPosition;
		return true;
	}
};
