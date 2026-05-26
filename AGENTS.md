# AGENTS.md — Submit Audio / VCV Rack

## Taal

Antwoord altijd in het Nederlands, tenzij José expliciet om een andere taal vraagt.

## Project

Dit is de VCV Rack 2 plugin-repository voor Submit Audio.

Lokale plugin-map:

`~/Submit`

Rack SDK:

`~/Rack-SDK`

Lokale build/install:

`cd ~/Submit && export RACK_DIR=~/Rack-SDK && make -j4 && make install`

## Technische waarheid

Gebruik altijd de lokale repository als technische waarheid.

Controleer bij twijfel altijd:

- `plugin.json`
- `src/plugin.cpp`
- `src/plugin.hpp`

De laatst bekende lokale versie tijdens setup is `2.15.2`, maar verifieer dit altijd opnieuw in `plugin.json` voordat je build-, versie- of releasewerk doet.

Oude Claude-documenten, oude Markdown-notities en exportbestanden zijn alleen achtergrond. Ze zijn geen waarheid als ze botsen met de huidige lokale repo of met José’s laatste instructie.

## VCV Rack 2 versie-regel

VCV Rack 2 vereist dat de pluginversie altijd begint met `2`.

Gebruik alleen:

`2.x.x`

Gebruik nooit:

`0.x.x` of `1.x.x`

## Module-consistentie

De modulelijst moet consistent zijn tussen:

- `plugin.json`
- `src/plugin.cpp`
- `src/plugin.hpp`
- `src/*.cpp` met `createModel`
- `res/*.svg` panelbestanden
- panelreferenties in de modulecode

Controleer altijd:

- elke slug in `plugin.json` heeft een bijbehorend `modelXXX` in `src/plugin.cpp`
- elke `p->addModel(modelXXX)` in `src/plugin.cpp` heeft een `extern Model* modelXXX;` in `src/plugin.hpp`
- elke module heeft een correcte `createModel<...>("Slug")`
- de slug in `createModel` komt exact overeen met de slug in `plugin.json`
- elke module verwijst naar een bestaand panelbestand
- panelpaden kloppen exact, inclusief hoofdletters

Let extra op hoofdletters. macOS kan fouten verbergen die Linux/GitHub Actions wel breken.

## Release modules

Deze modules gelden als normale release-modules, tenzij José iets anders zegt:

- Drift
- Chrono
- Impact
- Chain
- Squeeze
- Shape
- Master
- Gain
- Sweep
- Loop

## Beta modules

Deze modules zijn beta/lokaal/werk-in-uitvoering en mogen niet zomaar naar GitHub of in een release:

- Poly008
- Twin
- Clang
- Shortwave
- React
- ImpactBeta
- Void
- Swell

Beta modules mogen alleen naar GitHub of in een release als José dat expliciet zegt.

Sommige beta modules kunnen technisch grotendeels werken, maar nog wachten op panel design, visuele afwerking, handmatige tekst, finale testing, of José’s expliciete releasebeslissing. Behandel deze modules als beta totdat José expliciet zegt dat ze release-ready zijn.

Let op: vertrouw niet blind op bestaande beta-scripts. Controleer altijd ook handmatig de actuele lokale status.

## Kritieke release-regel

Voor elke release, commit, push, tag, plugin.json cleanup, GitHub-voorbereiding of ander releasegericht werk moet je altijd eerst vragen:

“Welke modules mogen in deze release?”

Ga pas verder nadat José dit expliciet heeft beantwoord.

Ga er nooit vanuit dat lokaal geregistreerde modules automatisch mee mogen naar GitHub.

## GitHub-regel

Push nooit naar GitHub tenzij José daar expliciet om vraagt.

Voer geen release-, tag-, commit- of push-acties uit op basis van aannames.

José bepaalt zelf wanneer er gepusht wordt.

## Dirty worktree regel

Controleer vóór wijzigingen altijd:

`git status --short`

Als er bestaande wijzigingen zijn, behandel die als werk van José.

Overschrijf, verplaats of verwijder bestaande wijzigingen niet zonder expliciete opdracht.

## Diff na edits

Na elke wijziging aan bestanden altijd de diff tonen:

`git diff`

Leg kort uit:

- welke bestanden zijn aangepast
- waarom ze zijn aangepast
- of er nog build/test nodig is

## Niet bewerken zonder expliciete opdracht

Bewerk, verwijder, hernoem of commit deze bestanden/mappen niet zonder expliciete opdracht:

- `build/`
- `dist/`
- `plugin.dylib`
- `backups/`
- `metamodule/build/`
- gegenereerde bestanden
- backupbestanden zoals `*.bak`, `*.bak2`, `*.backup_*`, `*_backup_*`

## Backup-regel

Backups worden standaard buiten de repository opgeslagen in:

`~/Submit_build_backups/`

Tussentijdse backups tijdens ontwikkeling:
- Maak alleen een backup als José daar expliciet om vraagt.
- Maak dan alleen een backup van de module waaraan op dat moment gewerkt wordt.
- Sla deze modulebackups op in:

`~/Submit_build_backups/module-backups/`

Einde-sessie backups:
- Aan het einde van een volledige ontwikkelsessie maakt Codex alleen een backup als José daar expliciet om vraagt.
- Deze backup bevat alles wat in die sessie is gewijzigd.
- Sla deze sessiebackups op in:

`~/Submit_build_backups/session-backups/`

Belangrijk:
- Maak geen backups in `~/Submit/`, tenzij José dat expliciet vraagt.
- Maak geen automatische backups zonder opdracht.
- Backupmappen mogen aangemaakt worden wanneer nodig.
- Backupnamen moeten duidelijk zijn met module-naam en datum/tijd.
- Voorbeelden:
  - `~/Submit_build_backups/module-backups/ImpactBeta_2026-05-26_1145/`
  - `~/Submit_build_backups/module-backups/React_2026-05-26_1210/`
  - `~/Submit_build_backups/session-backups/session_2026-05-26_end/`

## VCV Rack en MetaModule scheiden

Houd VCV Rack-werk en MetaModule-werk strikt gescheiden.

VCV Rack hoofdcode:

`~/Submit/src`

VCV Rack assets:

`~/Submit/res`

MetaModule werk staat apart in:

`~/Submit/metamodule`

Wijzig VCV Rack en MetaModule niet tegelijk, tenzij José daar expliciet om vraagt.

## MetaModule test-regel

Wanneer een module voor MetaModule wordt gecodeerd of aangepast, moet deze altijd eerst in de MetaModule simulator getest worden.

Werkwijze:
- Houd VCV Rack-code en MetaModule-code gescheiden.
- Bouw eerst de MetaModule-versie.
- Test daarna altijd in de MetaModule simulator voordat iets als werkend wordt beschouwd.
- Controleer in de simulator minimaal:
  - laadt de module zonder crash
  - panel verschijnt correct
  - knoppen/sliders reageren goed
  - inputs en outputs werken zoals bedoeld
  - audio/CV loopt zonder duidelijke glitches
  - geen ontbrekende assets of verkeerde paden
- Beschouw hardware-testen pas als volgende stap na een geslaagde simulator-test.
- Noem een MetaModule-aanpassing niet “klaar” zonder simulator-test, tenzij José expliciet zegt dat testen mag worden overgeslagen.

## MetaModule simulator command

Voor MetaModule-tests wordt de lokale simulator hier gebruikt:

`/Users/studio67/metamodule-main-git/simulator`

De simulator-binary staat hier:

`/Users/studio67/metamodule-main-git/simulator/build/simulator`

Gebruik standaard dit startcommando:

`cd ~/metamodule-main-git/simulator && ./build/simulator --audioout 1`

Let op: `--audioout 1` is belangrijk voor het starten met de juiste audio/soundcard output. Verander dit niet zomaar.

Als de simulator opnieuw gebouwd moet worden:

`cd ~/metamodule-main-git/simulator && cmake --fresh -B build -GNinja && cmake --build build`

Of via Makefile:

`cd ~/metamodule-main-git/simulator && make`

Belangrijk:
- Controleer vóór MetaModule-tests welke pluginbron de simulator gebruikt.
- De huidige lokale simulatorconfig gebruikt `/Users/studio67/Submit-MM` via `ext-plugins.cmake`.
- Ga er dus niet automatisch vanuit dat de simulator `~/Submit/metamodule` gebruikt.
- Als een MetaModule-aanpassing in `~/Submit/metamodule` is gedaan, moet eerst gecontroleerd worden of die versie ook werkelijk in de simulator-build zit.
- Noem MetaModule-werk pas werkend na een geslaagde simulator-test.

## Minimale verificatie voor modulewerk

Voor modulewerk altijd controleren:

- `plugin.json`
- `src/plugin.cpp`
- `src/plugin.hpp`
- `src/*.cpp`
- `res/*.svg`

Let vooral op:

- ontbrekende registraties
- extra registraties
- ontbrekende extern declarations
- verkeerde `createModel` slugs
- ontbrekende panel SVGs
- verkeerde hoofdletters in panelpaden
- beta modules die per ongeluk in releasewerk terechtkomen

## Bekende lokale aandachtspunten tijdens setup

Tijdens de eerste inventarisatie zijn deze aandachtspunten gevonden:

- `src/Swell.cpp` verwijst naar `res/Tide.svg`, maar `res/Swell.svg` bestaat.
- `src/Twin.cpp` verwijst naar `res/twiN.svg`, maar `res/Twin.svg` bestaat.
- `src/Sweep.cpp` gebruikt `res/Panel-design-sweep.svg`; `res/Sweep.svg` bestaat ook.
- `Drift` gebruikt historisch `Drift13.cpp` en `Drift13.svg`.
- `ImpactBeta` is beta en moet niet per ongeluk in releasewerk blijven staan.
