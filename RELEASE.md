# Submit — Release Procedure
> Volg deze stappen bij elke nieuwe release!

## NIEUWE WERKWIJZE
- Elke push naar master = alleen bouwen, geen release
- Release aanmaken = tag pushen: git tag v2.x.x && git push origin v2.x.x
- Release wordt als DRAFT aangemaakt — jij publiceert handmatig na controle

---

## STAP 1 — Verificatie uitvoeren

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

Beta modules (NOOIT pushen): Poly008, Twin, Shortwave, Void, Swell

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
Poly008, Twin, Shortwave, Void, Swell
