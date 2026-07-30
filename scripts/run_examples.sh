#!/usr/bin/env bash
# run_examples.sh — Build (optional) and run every GradiumPP example, then
# report a pass/fail summary.
#
# Requirements:
#   • GRADIUM_API_KEY environment variable must be set.
#   • cmake + a C++17 compiler (only needed when --build is passed).
#   • ffmpeg (only needed for asr_realtime; detected at runtime).
#
# Usage:
#   ./scripts/run_examples.sh [OPTIONS]
#
# Options:
#   --build               Run cmake configure + build before the examples.
#   --build-dir DIR       Path to the CMake build directory (default: build).
#   --voice ID            Voice ID used for TTS examples
#                         (default: NbpkqMVS3CJeq2j8 / voices::en::american::zoey).
#   --asr-wav FILE        WAV file supplied to asr_rest.  When omitted the
#                         script uses the WAV produced by tts_rest.
#   --asr-pcm FILE        Raw-PCM file (24 kHz, 16-bit, mono) supplied to
#                         asr_realtime.  When omitted the script tries to
#                         convert the asr_rest WAV with ffmpeg; if ffmpeg is
#                         absent the asr_realtime example is skipped.
#   --skip EXAMPLE,...    Comma-separated list of example names to skip.
#                         Valid names: voices tts_rest tts_realtime
#                                      tts_multiplex asr_rest asr_realtime
#                                      s2s_realtime
#   -h, --help            Show this help and exit.

set -euo pipefail

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------
BUILD=false
BUILD_DIR="build"
VOICE_ID="NbpkqMVS3CJeq2j8"   # voices::en::american::zoey
ASR_WAV=""
ASR_PCM=""
SKIP_LIST=""

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --build)        BUILD=true;       shift ;;
        --build-dir)    BUILD_DIR="$2";   shift 2 ;;
        --voice)        VOICE_ID="$2";    shift 2 ;;
        --asr-wav)      ASR_WAV="$2";     shift 2 ;;
        --asr-pcm)      ASR_PCM="$2";     shift 2 ;;
        --skip)         SKIP_LIST="$2";   shift 2 ;;
        -h|--help)
            awk '/^# run_examples/,/^set -/' "$0" \
                | grep -v '^set -' | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/$BUILD_DIR"

PASS=0
FAIL=0
SKIP=0
declare -a RESULTS=()

should_skip() {
    local name="$1"
    [[ ",$SKIP_LIST," == *",$name,"* ]]
}

run_example() {
    local name="$1"
    shift
    local bin="$BUILD_DIR/examples/$name/gradium_$name"

    if should_skip "$name"; then
        echo "  [SKIP] $name"
        RESULTS+=("SKIP  $name")
        (( SKIP++ )) || true
        return
    fi

    if [[ ! -x "$bin" ]]; then
        echo "  [FAIL] $name — binary not found: $bin"
        RESULTS+=("FAIL  $name (binary not found)")
        (( FAIL++ )) || true
        return
    fi

    echo "  Running: $name $*"
    local tmp_out
    tmp_out="$(mktemp)"
    local exit_code=0
    "$bin" "$@" >"$tmp_out" 2>&1 || exit_code=$?

    if [[ $exit_code -eq 0 ]]; then
        echo "  [PASS] $name"
        RESULTS+=("PASS  $name")
        (( PASS++ )) || true
    else
        echo "  [FAIL] $name (exit $exit_code)"
        echo "         --- output ---"
        sed 's/^/         /' "$tmp_out"
        RESULTS+=("FAIL  $name (exit $exit_code)")
        (( FAIL++ )) || true
    fi
    rm -f "$tmp_out"
}

# ---------------------------------------------------------------------------
# Preflight checks
# ---------------------------------------------------------------------------
if [[ -z "${GRADIUM_API_KEY:-}" ]]; then
    echo "Error: GRADIUM_API_KEY is not set." >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Optional build step
# ---------------------------------------------------------------------------
if [[ "$BUILD" == true ]]; then
    echo "==> Configuring and building..."
    cmake -B "$BUILD_DIR" -S "$REPO_ROOT" -DGRADIUM_BUILD_EXAMPLES=ON
    cmake --build "$BUILD_DIR" --parallel
    echo
fi

# ---------------------------------------------------------------------------
# Temporary working directory for generated files
# ---------------------------------------------------------------------------
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

TTS_REST_WAV="$TMP_DIR/tts_output.wav"
TTS_RT_WAV="$TMP_DIR/tts_realtime_output.wav"
MUX_PREFIX="$TMP_DIR/tts_multiplex"

echo "==> Running examples..."
echo

# 1. voices — list only (no audio I/O)
run_example voices --list-only

# 2. tts_rest
run_example tts_rest \
    --text "Hello from the GradiumPP integration script." \
    --voice "$VOICE_ID" \
    --format pcm_16000 \
    --out "$TTS_REST_WAV"

# 3. tts_realtime
run_example tts_realtime \
    --text "Hello from the GradiumPP realtime script." \
    --voice "$VOICE_ID" \
    --format pcm_16000 \
    --out "$TTS_RT_WAV"

# 4. tts_multiplex
# The binary writes to fixed filenames in the CWD; redirect into TMP_DIR via pushd.
pushd "$TMP_DIR" > /dev/null
run_example tts_multiplex --voice "$VOICE_ID"
popd > /dev/null

# 5. asr_rest
ASR_WAV_IS_RAW_PCM=false
if [[ -z "$ASR_WAV" ]]; then
    if [[ -f "$TTS_REST_WAV" ]]; then
        ASR_WAV="$TTS_REST_WAV"
        ASR_WAV_IS_RAW_PCM=true
    fi
fi

if [[ -n "$ASR_WAV" ]]; then
    run_example asr_rest --file "$ASR_WAV" --format pcm_16000
else
    echo "  [SKIP] asr_rest — no WAV file available (tts_rest may have failed)"
    RESULTS+=("SKIP  asr_rest (no WAV file)")
    (( SKIP++ )) || true
fi

# 6. asr_realtime — needs raw PCM (24 kHz, 16-bit, mono)
if [[ -z "$ASR_PCM" ]]; then
    if command -v ffmpeg &>/dev/null && [[ -f "$ASR_WAV" ]]; then
        PCM_FILE="$TMP_DIR/asr_input.pcm"
        if [[ "$ASR_WAV_IS_RAW_PCM" == true ]]; then
            # Headerless raw PCM from tts_rest --format pcm_16000, not a real WAV container.
            ffmpeg -y -f s16le -ar 16000 -ac 1 -i "$ASR_WAV" -ar 24000 -ac 1 -f s16le "$PCM_FILE" \
                </dev/null >/dev/null 2>&1 && ASR_PCM="$PCM_FILE"
        else
            ffmpeg -y -i "$ASR_WAV" -ar 24000 -ac 1 -f s16le "$PCM_FILE" \
                </dev/null >/dev/null 2>&1 && ASR_PCM="$PCM_FILE"
        fi
    fi
fi

if [[ -n "$ASR_PCM" && -f "$ASR_PCM" ]]; then
    run_example asr_realtime --file "$ASR_PCM" --format pcm
else
    echo "  [SKIP] asr_realtime — no raw-PCM file available" \
         "(provide --asr-pcm or install ffmpeg)"
    RESULTS+=("SKIP  asr_realtime (no PCM file; install ffmpeg or pass --asr-pcm)")
    (( SKIP++ )) || true
fi

# 7. s2s_realtime — reuses the same raw-PCM input as asr_realtime
S2S_PCM_OUT="$TMP_DIR/s2s_realtime_output.pcm"
if [[ -n "$ASR_PCM" && -f "$ASR_PCM" ]]; then
    run_example s2s_realtime \
        --file "$ASR_PCM" \
        --voice "$VOICE_ID" \
        --input-format pcm \
        --output-format pcm_16000 \
        --target-language en \
        --out "$S2S_PCM_OUT"
else
    echo "  [SKIP] s2s_realtime — no raw-PCM file available" \
         "(provide --asr-pcm or install ffmpeg)"
    RESULTS+=("SKIP  s2s_realtime (no PCM file; install ffmpeg or pass --asr-pcm)")
    (( SKIP++ )) || true
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo
echo "==> Results"
echo "-------------------------------------------"
for r in "${RESULTS[@]}"; do
    echo "  $r"
done
echo "-------------------------------------------"
echo "  Passed: $PASS  Failed: $FAIL  Skipped: $SKIP"
echo

if [[ $FAIL -gt 0 ]]; then
    exit 1
fi
