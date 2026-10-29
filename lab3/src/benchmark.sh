#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
gcc -std=c11 -O2 -pthread main.c -o dice

cases=(
    "1 0 0 0 1000000"
    "10 4 30 25 1000000"
    "10 4 30 25 10000000"
)

for data in "${cases[@]}"; do
    read -r rounds completed score1 score2 experiments <<< "$data"
    printf '\nДанные: %s\n' "$data"
    echo "Потоки   Замер 1   Замер 2   Замер 3   Медиана   Ускорение   Эффективность"

    for threads in 1 2 4 8 16; do
        times=()
        for run in 1 2 3; do
            output=$(./dice "$rounds" "$completed" "$score1" "$score2" "$experiments" --threads "$threads" --seed 42)
            elapsed=$(echo "$output" | awk '/^Время выполнения:/ {print $3}')
            times+=("$elapsed")
        done
        median=$(printf '%s\n' "${times[@]}" | sort -n | sed -n '2p')
        if (( threads == 1 )); then baseline=$median; fi
        awk -v p="$threads" -v a="${times[0]}" -v b="${times[1]}" -v c="${times[2]}" -v t="$median" -v t1="$baseline" \
            'BEGIN { if (t <= 0) exit 1; printf "%6d   %8.6f  %8.6f  %8.6f  %8.6f   %8.3f   %8.1f%%\n", p, a, b, c, t, t1/t, 100*t1/t/p }'
    done
done
