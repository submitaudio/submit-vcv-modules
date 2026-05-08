# Submit — Release & Push Procedure
> Altijd volgen bij elke push naar GitHub — ook voor kleine wijzigingen!

## GOUDEN REGEL
Elke push naar master triggert een nieuwe GitHub Actions build en release.
Dit geldt ook voor README wijzigingen, LICENSE, of andere kleine bestanden.
Nooit zomaar git push zonder deze procedure te volgen!

---

## STAP 1 — Verwijder beta modules

```bash
python3 ~/Submit/scripts/verwijder_beta.py
```

Verwijdert Poly008, twiN en Reel uit plugin.json, plugin.cpp en plugin.hpp.

---

## STAP 2 — Verificatie uitvoeren

```bash
python3 << 'EOF'
import json, re
with open('/Users/studio67/Submit/plugin.json') as f:
    data = json.load(f)
slugs = [m['slug'] for m in data['modules']]
beta = ['Poly008', 'twiN', 'Reel']
print('plugin.json:', slugs)
still_beta = [s for s in slugs if s in beta]
if still_beta:
    print('STOP — NOG BETA MODULES:', still_beta)
else:
    print('OK — Geen beta modules, klaar voor push!')
EOF
```

---

## STAP 3 — Versie verhogen (alleen bij nieuwe release)

Pas versie aan in plugin.json — altijd 2.x.x

---

## STAP 4 — Push naar GitHub

```bash
cd ~/Submit && git add . && git commit -m "Beschrijving" && git push
```

---

## STAP 5 — Herstel beta modules direct na push

```bash
python3 ~/Submit/scripts/herstel_beta.py
```

---

## STAP 6 — Lokaal bouwen met beta modules

```bash
cd ~/Submit && export RACK_DIR=~/Rack-SDK && make -j4 && make install
```

---

## STAP 7 — Controleer release op GitHub

```bash
curl -s https://api.github.com/repos/submitaudio/submit-vcv-modules/releases/latest | python3 -c "
import json,sys
r = json.load(sys.stdin)
print('Release:', r['tag_name'])
print('Pre-release:', r['prerelease'])
for a in r.get('assets', []):
    print(' ', a['name'])
"
```

Controleer: Pre-release = False, drie assets aanwezig.

---

## BETA MODULES

| Module  | Slug   | Model        |
|---------|--------|--------------|
| Poly008 | Poly008 | modelPoly008 |
| twiN    | twiN   | modelTwiN    |
| Reel    | Reel   | modelReel    |

Scripts: ~/Submit/scripts/verwijder_beta.py en herstel_beta.py

---

## NIEUWE BETA MODULE TOEVOEGEN

1. Voeg toe aan .gitignore:
   echo "src/NieuweModule.cpp" >> ~/Submit/.gitignore
   echo "res/NieuweModule.svg" >> ~/Submit/.gitignore

2. Voeg toe aan ~/Submit/scripts/verwijder_beta.py en herstel_beta.py

3. Registreer in plugin.hpp, plugin.cpp, plugin.json

---

## OFFICIELE MODULES
Drift, Chrono, Impact, Chain, Squeeze, Shape, Master, Gain, Sweep

## BETA MODULES (nooit pushen)
Poly008, twiN, Reel
