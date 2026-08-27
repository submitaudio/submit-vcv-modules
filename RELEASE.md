# Submit — Release Procedure
> Volg deze stappen bij elke nieuwe release!

## NIEUWE WERKWIJZE
- Elke push naar master = alleen bouwen, geen release
- Release aanmaken = tag pushen: git tag v2.x.x && git push origin v2.x.x
- Release wordt als DRAFT aangemaakt — jij publiceert handmatig na controle

---

## STAP 1 — Verificatie uitvoeren

### Harde regel: gebruik uitsluitend officiële VCV Rack-tags

De waarden in `plugin.json` onder `modules[].tags` moeten **letterlijk en exact**
voorkomen in de officiële VCV Rack-taglijst:

https://raw.githubusercontent.com/VCVRack/Rack/v2/src/tag.cpp

- Verzin nooit zelf een tag, ook niet wanneer de naam technisch logisch klinkt.
- Combineer of herformuleer bestaande tags niet.
- Let op hoofdletters, spaties en enkelvoud/meervoud: de waarde moet exact gelijk zijn.
- Gebruik bij twijfel een bestaande officiële tag die het dichtst bij de functie ligt.
- Als een nieuwe tag echt nodig is, vraag deze eerst aan via `support@vcvrack.com` en
  gebruik hem pas nadat hij officieel aan de Rack-taglijst is toegevoegd.

Voorbeeld: `Clock divider` klinkt logisch, maar is geen geldige VCV-tag.
Gebruik voor een clock divider/multiplier de officiële tag `Clock modulator`.

Controleer vóór iedere release alle gebruikte tags met:

```bash
comm -23 \
  <(jq -r '.modules[].tags[]' plugin.json | sort -u) \
  <(curl -fsSL https://raw.githubusercontent.com/VCVRack/Rack/v2/src/tag.cpp \
    | sed -n 's/.*{"\([^"]*\)".*/\1/p' \
    | sort -u)
```

Geen uitvoer betekent dat alle tags geldig zijn. Iedere regel die wel wordt
getoond is een ongeldige tag en moet vóór commit, push of VCV-aanmelding worden
vervangen.

## DOCUMENTATIE BIJ ELKE WIJZIGING

Documentatie hoort bij dezelfde wijziging als de code. Werk daarom tijdens de
ontwikkeling direct de volgende bestanden bij:

- `docs/changelogs/<Module>.md`: voeg iedere gebruikersrelevante wijziging toe
  onder `Unreleased`. Noteer minimaal wat er is veranderd, of bestaande patches
  compatibel blijven en welke test nog nodig is.
- `CHANGELOG.md`: houd het algemene release-overzicht en de links naar alle
  modulechangelogs actueel. Verplaats bij een release de relevante punten van
  `Unreleased` naar het nieuwe versienummer.
- `MANUAL.md`: voeg een nieuwe publieke module toe aan de inhoudsopgave en geef
  een actuele beschrijving van controls, inputs, outputs en een eerste patch.
- `plugin.json`: controleer bij iedere publieke module de naam, slug,
  beschrijving, `manualUrl` en tags. Laat `pluginUrl`, `manualUrl` en
  `changelogUrl` naar de vaste publieke pagina's wijzen.

### Nieuwe publieke module

Een nieuwe module is pas compleet gedocumenteerd wanneer deze tegelijk in de
volgende onderdelen staat:

1. `plugin.json`
2. `src/plugin.cpp` en `src/plugin.hpp`
3. `MANUAL.md`
4. `docs/changelogs/<Module>.md`
5. de modulelijst in `README.md` wanneer de module publiek wordt uitgebracht
6. het Rack-contextmenu met een directe `Changelog`-link

Maak voor iedere nieuwe module meteen een changelogbestand aan, ook als de
eerste versie alleen een `Unreleased`-sectie bevat. Zo raakt geen enkele
finetuning zoek en kunnen community-updates rechtstreeks uit die punten worden
geschreven.

Beta-modules mogen lokaal een manual en changelog hebben, maar worden niet in
de publieke GitHub-manual, release of VCV-aanmelding opgenomen totdat José ze
expliciet heeft goedgekeurd.

### Verplicht bij iedere nieuwe module

**José, vergeet niet de CPU-stresstest te doen!**

- Test de module in VCV Rack met snelle triggers, maximale decay en meerdere overlappende stemmen.
- Controleer zowel het normale CPU-gebruik als korte pieken en vergelijk meerdere modellen/presets.
- Test een MetaModule-versie na de simulator ook op de echte MetaModule-hardware.
- Breng een nieuwe module pas uit nadat klank, polyfonie en CPU-belasting zijn goedgekeurd.

```bash
python3 << 'EOF'
import json
with open('/Users/studio67/SubmitAudio-Development/Projects/VCV-Rack/plugin.json') as f:
    data = json.load(f)
json_slugs = [m['slug'] for m in data.get('modules', [])]
with open('/Users/studio67/SubmitAudio-Development/Projects/VCV-Rack/src/plugin.cpp') as f:
    cpp = f.read()
with open('/Users/studio67/SubmitAudio-Development/Projects/VCV-Rack/src/plugin.hpp') as f:
    hpp = f.read()
print('=== MODULE VERIFICATIE ===')
print('plugin.json:', json_slugs)
all_ok = True
for slug in json_slugs:
    model = f'model{slug}'
    in_cpp = model in cpp
    in_hpp = model in hpp
    status = '✅' if (in_cpp and in_hpp) else '❌'
    print(f'{status} {slug}: hpp={"✅" if in_hpp else "❌"}  cpp={"✅" if in_cpp else "❌"}')
    if not (in_cpp and in_hpp):
        all_ok = False
print()
print('RESULTAAT:', '✅ Alles OK' if all_ok else '❌ FIX NODIG VOOR BUILD!')
EOF
```

---

## STAP 2 — Controleer exact de publieke modulelijst

De publieke release bevat uitsluitend: Drift, Chrono, Impact, Chain, SumM4,
SumS4, Tag, Set, Pulse, Squeeze, Shape, Master, Gain, Sweep, Loop, Clang,
React, Sync, Flip, Orbit, Circles en Sub.

Beta/local modules (NOOIT pushen zonder expliciete goedkeuring): Poly008, Twin,
Shortwave, Void, VoidV2, VoidV3, Swell en Machina.

Gebruik `scripts/verwijder_beta.py`, `scripts/herstel_beta.py` en
`scripts/release.py` niet voor deze release. Deze oude scripts kennen de actuele
modulelijst niet en kunnen lokaal werk verwijderen of onbedoeld committen en pushen.
Bereid een release altijd in een aparte release-worktree voor, zodat de volledige
lokale beta-opstelling in `/Users/studio67/SubmitAudio-Development/Projects/VCV-Rack` behouden blijft.

Maak en controleer eerst een veilige releasekopie:

```bash
release_dir="$(mktemp -d /private/tmp/submit-release.XXXXXX)"
rmdir "$release_dir"
python3 release-tools/prepare_release_copy.py "$release_dir"
cd "$release_dir"
export RACK_DIR=/Users/studio67/SubmitAudio-Development/Toolchains/Rack-SDK
make -j4 && make dist
```

Het hulpscript weigert een bestaande doelmap te overschrijven, voert geen
Git-acties uit en controleert de publieke slugs, registraties en panel-assets.

---

## STAP 3 — Versie verhogen in plugin.json (altijd 2.x.x)

---

## STAP 4 — Maak een gecontroleerde publieke commit en push

Gebruik **nooit** blind `git add .` vanuit de lokale ontwikkelmap. Daar staan ook
beta-modules en ander lokaal werk. Maak de publieke commit alleen vanuit een
gecontroleerde release-worktree waarin de inhoud overeenkomt met de hierboven
geteste releasekopie. Controleer vóór de commit opnieuw dat uitsluitend de 22
goedgekeurde modules geregistreerd zijn.

```bash
git status --short
git diff --cached
git commit -m "Release Submit 2.x.x"
git push
```

Noteer na de push de volledige commit-hash:

```bash
git rev-parse HEAD
```

---

## STAP 5 — Release aanmaken via tag

```bash
cd /Users/studio67/SubmitAudio-Development/Projects/VCV-Rack && git tag v2.x.x && git push origin v2.x.x
```

---

## STAP 6 — Controleer draft release op GitHub

Ga naar: github.com/submitaudio/submit-vcv-modules/releases

Controleer:
- win-x64 aanwezig?
- lin-x64 aanwezig?
- mac-arm64 aanwezig?
- Juiste versienummer?
- Geen beta modules?

Dan publiceren!

---

## STAP 7 — Meld de update in het permanente VCV Library-issue

Voor Submit wordt altijd het bestaande issue gebruikt:

https://github.com/VCVRack/library/issues/905

Maak geen nieuw VCV Library-issue. Plaats na de definitieve push een comment met
het exacte versienummer en de **volledige commit-hash**. Geef niet alleen een
branchnaam zoals `master`. De VCV-maintainer heropent het issue, verwerkt de
update en sluit het weer wanneer de build beschikbaar is.

Bericht voor deze release:

```text
Submit 2.20.0 is ready for the VCV Library.

Version: 2.20.0
Commit: <FULL_COMMIT_HASH>

This release expands the Submit Audio collection to 22 modules with Circles
and SUB. It also adds selectable 1 PPQN Submit-standard and 4 PPQN compatibility
timing to the relevant clocked modules.
```

Plaats dit bericht pas wanneer de publieke commit definitief op GitHub staat en
de drie platform-builds van dezelfde commit succesvol zijn gecontroleerd.

---

## OFFICIELE MODULES
Drift, Chrono, Impact, Chain, SumM4, SumS4, Tag, Set, Pulse, Squeeze, Shape, Master, Gain, Sweep, Loop, Clang, React, Sync, Flip, Orbit, Circles, Sub

## BETA MODULES (nooit pushen)
Poly008, Twin, Shortwave, Void, VoidV2, VoidV3, Swell, Machina
