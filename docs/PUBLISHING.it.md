# Pubblicare PTL-FreeRTOS sul tuo profilo GitHub

La cartella pronta da caricare è **PTL-FreeRTOS**, cioè la tua versione sistemata. Il nome può essere uguale a quello del tuo compagno perché la nuova repository sarà sul tuo profilo.

Questa copia non contiene `.git`, vecchi commit o collegamenti al remote del compagno. Conserva invece i crediti del gruppo, LICENSE, `.gitignore`, `.gitattributes` e il workflow `.github/`.

## 1. Crea la repository vuota su GitHub

Apri [New repository](https://github.com/new) dal tuo account e imposta:

| Campo | Valore |
| --- | --- |
| Owner | Il tuo profilo |
| Repository name | `PTL-FreeRTOS` |
| Description | `Periodic task scheduling, deadline monitoring and overrun handling on FreeRTOS for ARM Cortex-M3 / QEMU.` |
| Visibility | Public, se vuoi mostrarla nel portfolio; Private se vuoi condividerla in seguito |

Lascia disattivate le opzioni per aggiungere README, `.gitignore` e licenza: questi file sono già nella cartella. Premi **Create repository** e copia l'indirizzo HTTPS della nuova repository.

## 2. Apri il Terminale nella cartella corretta

Sul Mac attuale:

```bash
cd "/Users/riccardobartolini/Desktop/Uni/1St Semester/PTL-FreeRTOS"
```

Questa deve essere la cartella contenente direttamente `README.md`, `Project/`, `FreeRTOS/` e `docs/`. Non usare la cartella `PROGETTO CAOS COPIA`, né quella che contiene tutti i progetti universitari.

## 3. Crea la tua nuova cronologia

```bash
git init -b main
git add .
git status --short
```

Prima del commit, la lista deve comprendere i sorgenti, la documentazione, il PDF e le configurazioni della nuova repository. Log, cartelle Output e `main_gen.c` vengono esclusi automaticamente.

```bash
git commit -m "Initial import: PTL periodic task layer for FreeRTOS"
```

Questo sarà il primo commit della **tua** repository. Non importa i vecchi commit.

Se Git richiede nome ed email, impostali soltanto in questa repository, sostituendo i valori di esempio con quelli del tuo account, quindi ripeti il commit:

```bash
git config user.name "IL TUO NOME"
git config user.email "LA TUA EMAIL GITHUB O NOREPLY"
```

## 4. Collega il tuo profilo e carica

Sostituisci `TUO_USERNAME` con il tuo nome utente GitHub:

```bash
git remote add origin https://github.com/TUO_USERNAME/PTL-FreeRTOS.git
git remote -v
git push -u origin main
```

Controlla che `origin` punti al **tuo** profilo. Se occorre autenticarsi e usi GitHub CLI:

```bash
brew install gh
gh auth login
gh auth setup-git
```

Segui l'accesso dal browser per il tuo account, scegli HTTPS, poi ripeti `git push -u origin main`. Non serve inserire credenziali nel progetto.

## 5. Controlla la pagina pubblicata

Il README dovrebbe comparire automaticamente, con badge, diagramma e collegamenti alla documentazione. Nella scheda **Actions**, verifica il primo avvio del workflow: esegue i controlli Python, compila la demo e lancia i test QEMU. La verifica locale è già stata effettuata; il primo risultato sui runner GitHub deve ancora essere osservato.

Nella sezione **About**, puoi aggiungere i topic:

```text
freertos embedded-systems real-time-systems cortex-m3 qemu c periodic-tasks
```

Per modifiche future:

```bash
git add .
git commit -m "Describe the change"
git push
```

## Alternativa: creazione con GitHub CLI

Se preferisci evitare la creazione dal sito, esegui questo percorso **al posto** del punto 1 e del collegamento manuale del punto 4. Dopo aver inizializzato e creato il commit locale:

```bash
gh auth login
gh auth setup-git
gh repo create PTL-FreeRTOS --source=. --public --remote=origin --push
```

Usa `--private` al posto di `--public` se desideri una repository privata. Questo comando crea e carica la repository sul profilo autenticato.

## Riferimenti ufficiali

- [Aggiungere codice locale a GitHub](https://docs.github.com/en/migrations/importing-source-code/using-the-command-line-to-import-source-code/adding-locally-hosted-code-to-github)
- [Autenticazione con GitHub CLI](https://cli.github.com/manual/gh_auth_login)
- [Configurare Git con GitHub CLI](https://cli.github.com/manual/gh_auth_setup-git)
- [Creare una repository con GitHub CLI](https://cli.github.com/manual/gh_repo_create)
