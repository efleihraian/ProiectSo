#!/bin/bash

HUNT_DIR="./hunts/$1"

if [ ! -d "$HUNT_DIR" ]; then
    echo "Hunt '$1' does not exist."
    exit 1
fi

declare -A scores

for treasure in "$HUNT_DIR"/*; do
    if [ -f "$treasure" ]; then
        owner=$(grep -oP 'owner=\K[^,]+' "$treasure")
        value=$(grep -oP 'value=\K[0-9]+' "$treasure")

        if [ -n "$owner" ] && [ -n "$value" ]; then
            scores["$owner"]=$((scores["$owner"] + value))
        fi
    fi
done

echo "Scores for hunt '$1':"
for user in "${!scores[@]}"; do
    echo "$user: ${scores[$user]}"
done
