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
with open('/Users/studio67/Submit/plugin.json') as f:
    data = json.load(f)
json_slugs = [m['slug'] for m in data.get('modules', [])]
with open('/Users/studio67/Submit/src/plugin.cpp') as f:
    cpp = f.read()
with open('/Users/studio67/Submit/src/plugin.hpp') as f:
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

## STAP 2 — Controleer geen beta modules

Beta/local modules (NOOIT pushen zonder expliciete goedkeuring): Poly008, Twin,
Shortwave, Void, VoidV2, VoidV3, Swell en Circles.

---

## STAP 3 — Versie verhogen in plugin.json (altijd 2.x.x)

---

## STAP 4 — Push naar GitHub

```bash
cd ~/Submit && git add . && git commit -m "Beschrijving" && git push
```

---

## STAP 5 — Release aanmaken via tag

```bash
cd ~/Submit && git tag v2.x.x && git push origin v2.x.x
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

## OFFICIELE MODULES
Drift, Chrono, Impact, Chain, Squeeze, Shape, Master, Gain, Sweep, Loop, Clang, React

## BETA MODULES (nooit pushen)
Poly008, Twin, Shortwave, Void, VoidV2, VoidV3, Swell, Circles
