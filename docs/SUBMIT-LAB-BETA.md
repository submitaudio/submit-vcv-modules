# Submit Lab beta-afspraken

Submit Lab is het afzonderlijke openbare betakanaal voor Circles en SUB.
De actieve Submit-repository blijft de enige bron van waarheid; er wordt geen
tweede handmatig onderhouden kopie van de modulecode gemaakt.

## Identiteit

- pluginnaam: `Submit Lab`
- plugin-slug: `SubmitLab`
- eerste betaversie: `2.0.0`
- actuele betakandidaat: `2.0.1` (Beta 2)
- modules: `Circles` en `SUB`
- moduleslugs: `Circles` en `Sub`
- geen automatische opname in de VCV Rack Library

Submit Lab kan naast de normale plugin met slug `Submit` worden geïnstalleerd.
Een patch onderscheidt `SubmitLab/Circles` van `Submit/Circles`; daarom blijft
de laatste Submit Lab-build later beschikbaar en wordt vóór de normale release
een veilige patchmigratie ontworpen en getest.

## Reproduceerbare build

Maak altijd een nieuwe tijdelijke buildkopie:

```bash
lab_dir="$(mktemp -d /private/tmp/submit-lab.XXXXXX)"
rmdir "$lab_dir"
python3 release-tools/prepare_submit_lab_copy.py "$lab_dir" --version 2.0.1
cd "$lab_dir"
export RACK_DIR=/Users/studio67/SubmitAudio-Development/Toolchains/Rack-SDK
make -j4
make dist
```

De builder voert geen Git-acties uit, weigert een bestaande doelmap te
overschrijven en laat uitsluitend Circles, SUB en hun expliciete assets toe.

## Permanente beta-links

De genummerde `submitlab-v*`-tags bewaren de beta-historie. Iedere toegestane
beta-publicatie werkt daarnaast de rolling prerelease `submitlab-beta` bij met
vaste assetnamen. Daardoor hoeven website-downloadlinks niet per beta te worden
aangepast.

- releasepagina: `https://github.com/submitaudio/submit-vcv-modules/releases/tag/submitlab-beta`
- macOS: `https://github.com/submitaudio/submit-vcv-modules/releases/download/submitlab-beta/SubmitLab-mac-arm64.vcvplugin`
- Windows: `https://github.com/submitaudio/submit-vcv-modules/releases/download/submitlab-beta/SubmitLab-win-x64.vcvplugin`
- Linux: `https://github.com/submitaudio/submit-vcv-modules/releases/download/submitlab-beta/SubmitLab-lin-x64.vcvplugin`

## Feedbackroutes

- algemene informatie, klankfeedback en ideeën: bestaande VCV Community-draad;
- reproduceerbare bugs: GitHub Issues;
- downloads en wijzigingen: GitHub prereleases;
- alle publicatieacties vereisen José's expliciete toestemming.

## Compatibiliteitscontract

Vanaf de eerste openbare beta worden bestaande parameter-, input-, output- en
light-ID's niet herschikt. Patchopslag blijft tussen betaversies leesbaar. Een
wijziging die bestaande patches kan veranderen wordt eerst expliciet besproken,
gedocumenteerd en met oudere beta-patches getest.
