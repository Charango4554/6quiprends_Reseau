set -e

if [ $# -lt 1 ]; then
    echo "Usage : $0 <id_partie> [fichier_sortie.pdf]" >&2
    exit 1
fi

PARTIE_ID="$1"

# Répertoire du script (ici src/) puis répertoire stats/ et logs.csv
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STATS_DIR="$SCRIPT_DIR/.."
LOGS_FILE="$STATS_DIR/logs.csv"

if [ ! -f "$LOGS_FILE" ]; then
    echo "Erreur : fichier de logs introuvable : $LOGS_FILE" >&2
    exit 1
fi

if [ ! -f "$STATS_DIR/stats_globales.awk" ] || [ ! -f "$STATS_DIR/ranking.awk" ]; then
    echo "Erreur : scripts AWK manquants dans $STATS_DIR" >&2
    exit 1
fi


if ! command -v awk >/dev/null 2>&1; then
    echo "Erreur : awk n'est pas disponible sur ce système." >&2
    exit 1
fi

if ! command -v pdflatex >/dev/null 2>&1; then
    echo "Erreur : pdflatex n'est pas disponible. Installez LaTeX pour générer le PDF." >&2
    exit 1
fi

# Fichier de sortie PDF (par défaut dans stats/)
DEFAULT_PDF="$SCRIPT_DIR/partie_${PARTIE_ID}_stats.pdf"
OUTPUT_PDF="${2:-$DEFAULT_PDF}"

# Dossier temporaire de travail
TMP_DIR="$(mktemp -d)"
PART_LOG="$TMP_DIR/logs_partie_${PARTIE_ID}.csv"
GLOBAL_STATS="$TMP_DIR/stats_globales_${PARTIE_ID}.txt"
RANKING="$TMP_DIR/ranking_${PARTIE_ID}.txt"
TEX_FILE="$TMP_DIR/rapport_partie_${PARTIE_ID}.tex"

# 1) Filtrer les logs pour ne garder que la partie demandée
awk -F';' -v pid="$PARTIE_ID" '$1 == pid' "$LOGS_FILE" > "$PART_LOG"

if [ ! -s "$PART_LOG" ]; then
    echo "Aucune entrée trouvée pour la partie $PARTIE_ID dans $LOGS_FILE" >&2
    rm -rf "$TMP_DIR"
    exit 1
fi

# 2) Générer les stats globales et le classement avec les AWK existants
awk -F';' -f "$STATS_DIR/stats_globales.awk" "$PART_LOG" > "$GLOBAL_STATS"
awk -F';' -f "$STATS_DIR/ranking.awk" "$PART_LOG" > "$RANKING"

# 3) Générer un petit document LaTeX
cat > "$TEX_FILE" <<EOF
\documentclass[a4paper,11pt]{article}
\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage[french]{babel}
\usepackage{geometry}
\geometry{margin=2cm}
\title{Statistiques de la partie $PARTIE_ID}
\date{}
\begin{document}
\maketitle

\section*{Statistiques globales par joueur}
\begin{verbatim}
$(cat "$GLOBAL_STATS")
\end{verbatim}

\section*{Classement des joueurs}
\begin{verbatim}
$(cat "$RANKING")
\end{verbatim}

\end{document}
EOF

# 4) Compilation LaTeX -> PDF
(
    cd "$TMP_DIR"
    pdflatex -interaction=nonstopmode "$(basename "$TEX_FILE")" >/dev/null 2>&1
)

PDF_TMP="$TMP_DIR/rapport_partie_${PARTIE_ID}.pdf"
if [ ! -f "$PDF_TMP" ]; then
    echo "Erreur lors de la génération du PDF." >&2
    rm -rf "$TMP_DIR"
    exit 1
fi

# 5) Déplacement du PDF à l'endroit souhaité
mkdir -p "$(dirname "$OUTPUT_PDF")"
mv "$PDF_TMP" "$OUTPUT_PDF"

echo "PDF généré : $OUTPUT_PDF"

# Nettoyage des fichiers temporaires
rm -rf "$TMP_DIR"

