# Confronto e integrazione delle due copie

Confronto effettuato l'11 ottobre 2026 tra la copia locale di Riccardo (`Progetto CAOS`) e la copia scaricata dal repository condiviso `Gianmarco-Fabbri/PTL-FreeRTOS`. La cartella preparata per la pubblicazione è stata rinominata `PTL-FreeRTOS`.

Il riferimento Git scaricato è il commit `440e3d9db7338911987d468d6062d25ccd3cef8f`, datato 8 febbraio 2026. Riccardo ha confermato di aver scaricato quella copia al momento del confronto. Non è stato necessario trasferirne la cronologia nel progetto da pubblicare.

## Risultato del confronto

Le copie non erano semplicemente una versione vecchia e una nuova: ciascuna conteneva elementi utili e alcune differenze erano regressioni. La copia locale è stata usata come base, integrando il materiale utile dell'altra senza sovrascrivere indiscriminatamente i sorgenti.

| Componente | Differenze osservate | Scelta per la versione finale |
| --- | --- | --- |
| Kernel FreeRTOS | File presenti nelle due copie identici byte per byte; `task.h` dichiara V11.1.0+ | Conservati; aggiunta copia della licenza già presente nei loro header |
| Scheduler PTL | Logica sostanzialmente comune; la copia Git incrementa ulteriormente alcuni contatori CATCH_UP | Conservato l'approccio locale; documentati i limiti di conteggio e dei job pendenti |
| Wrapper PTL | Il locale evita un secondo conteggio alla fine di un job già marcato in ritardo | Conservato; aggiunta validazione preventiva di tutta la configurazione e rollback delle creazioni precedenti se una creazione fallisce e restituisce errore |
| Configurazione FreeRTOS | Il locale abilita esplicitamente time slicing e hook di allocazione fallita | Conservata con l'hook di memoria presente nel locale |
| Sicurezza inizializzazione | Il locale termina il controllo quando rileva un errore; l'altra copia prosegue verso PASS | Rafforzati i controlli; un risultato PASS non può nascondere un FAIL nel runner |
| `test_init_safety.c` | Presente solo nella copia Git; copertura in parte duplicata | Integrato e ampliato per ciclo di vita, deadline implicita e policy globale |
| Altri test | Alcuni usano busy-wait su tick nella copia Git e burner calibrato nel locale; carichi diversi | Conservati gli scenari locali, che passano con QEMU `-icount 1`; non ridotta artificialmente la difficoltà dei test |
| Test di produzione | Nella copia Git compare un PASS incondizionato dopo il controllo overhead | Conservata la condizione del locale, eliminata la possibilità di successo dopo errore nel runner |
| Statistiche | Entrambe le copie confondono percentuale idle e overhead nel riepilogo generico | Corretto il riepilogo; overhead stimato soltanto nel benchmark dedicato |
| Makefile | Entrambe condividono gli artefatti tra entry point; il locale nasconde gli errori di compilazione del runner | Output separati per entry point, dipendenze degli header, errori visibili, avvio QEMU con build automatica |
| Configuratore | Il locale limita già i task e gestisce meglio l'interruzione | Conservato e migliorato: nomi validi/univoci, timing verificato, preview limitata, rilevamento deadline dei job in attesa, sorgente conservato |
| `config_gen.py` | Presente solo nella copia Git; prototipo precedente senza generazione completa | Non duplicato: `framework.py` contiene il percorso completo di analisi e generazione |
| `main_gen.c` e log | Generati nella copia Git | Esclusi dalla pubblicazione; rigenerabili con i comandi documentati |
| Specifica del progetto | Presente solo nella copia Git | Integrata senza modifiche in `docs/ASSIGNMENT.md` |
| Licenza | File LICENSE presente solo nella copia Git | Integrato preservando l'attribuzione originale e gli header esistenti |
| Report PDF | Presente solo nel locale | Conservato byte per byte in `docs/Documentation.pdf` |
| README e modello temporale | Versione kernel errata, comandi/percorsi incoerenti, API inesistente `xPhase`, garanzie non dimostrate | Riscritti sulla base dei sorgenti effettivi |

## Miglioramenti di presentazione e manutenzione

- README in inglese con indice, badge statici, diagramma, demo, avvio rapido, API, test e risoluzione dei problemi.
- Documentazione raccolta in `docs/`, inclusa una guida alla pubblicazione in italiano.
- Comandi disponibili anche dalla radice, senza dover entrare manualmente in `Project/`.
- Dieci test firmware, sei controlli Python e workflow GitHub Actions.
- `.gitignore` aggiornato per artefatti, log, configurazioni generate e cache Python.
- Tabella dei vettori resa costante anche nei puntatori, eliminando il segmento ELF marcato contemporaneamente scrivibile ed eseguibile.
- Crediti dei quattro autori conservati; gli indirizzi email non sono necessari nel README pubblico.

## Cronologia e pulizia

Nella copia di Riccardo non erano presenti cartelle `.git` né repository annidati. La versione finale non contiene la cronologia del compagno. `.gitignore`, `.gitattributes` e `.github/` sono file di configurazione utili alla nuova repository, non una vecchia cronologia.

La copia scaricata del compagno è rimasta invariata. Un backup completo precedente alle modifiche si trova accanto al progetto, nella cartella `Backup CAOS`, e non fa parte della repository da caricare.

## Verifica e limiti

La suite originale locale ha riportato 9/9 scenari con il runner precedente. La versione integrata ha passato 10/10 scenari con il runner più rigoroso e 6/6 controlli Python. Ambiente e dettagli sono in [VALIDATION.md](VALIDATION.md).

Il progetto rimane un'implementazione accademica del modello originario. I limiti di CATCH_UP, deadline, wraparound e tracing sono esplicitati in [TIMING_MODEL.md](TIMING_MODEL.md); il riordino non viene presentato come una certificazione di correttezza per ogni carico o piattaforma.
