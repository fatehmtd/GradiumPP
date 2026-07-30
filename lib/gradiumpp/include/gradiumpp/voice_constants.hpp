#pragma once

// Flagship catalog voice IDs from https://docs.gradium.ai/guides/voices/flagship-voices
// Usage: gradium::voices::<lang>::<dialect>::<name>
//
// This mirrors Gradium's current curated "flagship" list (66 voices across five
// languages). It is not the full voice catalog — use VoiceClient::listVoicesTyped(true)
// to enumerate every catalog voice available to your account, including non-flagship ones.

namespace gradium::voices {

// ── English ───────────────────────────────────────────────────────────────────

namespace en {

namespace american {

constexpr const char* zoey     = "NbpkqMVS3CJeq2j8";
constexpr const char* skyler   = "cLONiZ4hQ8VpQ4Sz";
constexpr const char* riley    = "7aEKz4P1ogZ0UsRP";
constexpr const char* quinn    = "vtG8ddh4IN32Otad";
constexpr const char* harper   = "4SZHfMpw-p46Ywgs";
constexpr const char* sterling = "6MFfc37kq0sBjBjy";
constexpr const char* russell  = "_6Aslh2DxfmnRLmP";
constexpr const char* marcus   = "r2sIQdqqoqgRJuXw";
constexpr const char* garrett  = "POBHtemksfWQbng0";
constexpr const char* damon    = "KUpE0JVhjiIzp1Fk";

} // namespace american

namespace british {

constexpr const char* tilly   = "4rdlkbxRv4m3UQTW";
constexpr const char* pippa   = "uem82D50GRv2Dwma";
constexpr const char* maeve   = "6PWnV0Nq4wu7RVBT";
constexpr const char* imogen  = "gDT1nz7Ie36ZhL-C";
constexpr const char* toby    = "dME3IWyZBvmh1n1q";
constexpr const char* reuben  = "CF0NgaMwHMMrHZn0";
constexpr const char* freddie = "s_k3kLBbgeK9-xUg";
constexpr const char* archie  = "kfzLbcdE_yXgLeUI";

} // namespace british

} // namespace en

// ── French ────────────────────────────────────────────────────────────────────

namespace fr {

namespace french {

constexpr const char* romane    = "jBULVCDhf05tOJN5";
constexpr const char* margaux   = "J8c9KBRYAGGYwjns";
constexpr const char* garance   = "3hQIj8JOo7bU31Jw";
constexpr const char* capucine  = "P4GqVY98hjQSvkiu";
constexpr const char* apolline  = "6oIkS98REoVZ1dEw";
constexpr const char* marius    = "biuhvu17TxVKOcyy";
constexpr const char* jules     = "YKeBw3OV1RgpdhLh";
constexpr const char* gaspard   = "iEu63s1rhn_kegTr";
constexpr const char* damien    = "25AzBFyp6svYnJsj";
constexpr const char* augustin  = "Tek4tJXiX6_yvXq7";

} // namespace french

namespace canadian {

constexpr const char* roxane      = "mmLFHtCjt_6jw0vT";
constexpr const char* frederique  = "sX0PcxM_Ie2ctBGH";
constexpr const char* maude       = "sBLwTd5womVX8JOw";

} // namespace canadian

} // namespace fr

// ── German ────────────────────────────────────────────────────────────────────

namespace de {

namespace germany {

constexpr const char* svenja  = "SqFfhmAgR2XdN83R";
constexpr const char* ronja   = "6XwZeudK_gn719g5";
constexpr const char* mila    = "nMNZ0sOWZVbKyjaI";
constexpr const char* marlene = "yHyu3PRfSmmiL2a4";
constexpr const char* annika  = "p6Uutkyi3j2iNAUu";
constexpr const char* mats    = "Kf5m22mROozoMWj3";
constexpr const char* leon    = "20zdyYrQPzKlCwkk";
constexpr const char* henrik  = "yyS1KYWs6mXoEw7D";
constexpr const char* erik    = "lbpBQTVCOcOHJ5zS";
constexpr const char* anton   = "3ZKKapPOvuWFcw9f";

} // namespace germany

namespace austria {

constexpr const char* sophie = "TXbEUrHXNFlYBBKb";
constexpr const char* stefan = "Xtp0vUDvtAfi1xkH";
constexpr const char* maxi   = "BPHlOW9jPs79KtW4";

} // namespace austria

} // namespace de

// ── Spanish ───────────────────────────────────────────────────────────────────

namespace es {

namespace spanish {

constexpr const char* vega    = "m3lIeODdTQ3bOh4z";
constexpr const char* paula   = "hP6WA-7ybEGApJ68";
constexpr const char* lucia   = "A3UKMLXQUzknYpQa";
constexpr const char* aitana  = "9UogMEa01dHR9Xbc";
constexpr const char* mateo   = "sVLgzKMqaptUdaY8";
constexpr const char* marcos  = "jvPx8j8zLGQ3utZz";
constexpr const char* iker    = "t-_TS1e-0GzDAX02";
constexpr const char* alvaro  = "ZeL1KGaZ4BZ2w0Np";

} // namespace spanish

namespace mexican {

constexpr const char* camila   = "4NLtOv1m0azv9rGL";
constexpr const char* ximena   = "VDwnGxAo68C8U8vC";
constexpr const char* regina   = "s58clrg2fe0MO7Y-";
constexpr const char* santiago = "yHToO6ssaQHz5kIP";
constexpr const char* diego    = "n7vovxcDTVG4gClo";
constexpr const char* emiliano = "tWll9uiMafMXfOGw";

} // namespace mexican

} // namespace es

// ── Portuguese ────────────────────────────────────────────────────────────────

namespace pt {

namespace brazilian {

constexpr const char* yara    = "YnWEONxJy7ptGhfb";
constexpr const char* manuela = "fd7e1fLVAAJzzs8P";
constexpr const char* larissa = "DUFZMTh4n-53Ly9O";
constexpr const char* helena  = "ycGyxoEy9wLaX11R";
constexpr const char* bianca  = "uCqxlQCKi8sPHwG2";
constexpr const char* mateus  = "AByHrwi1S-yLzW-s";
constexpr const char* davi    = "NuUr_x5V90hSHzCJ";
constexpr const char* caio    = "Qit9Oc9fEO9yXsVw";

} // namespace brazilian

} // namespace pt

} // namespace gradium::voices
