#!/bin/sh
# Runs every tests/<group>/<name>.ell with the elvm in compiler/ and compares
# what it prints (and its exit code) with <name>.expected. A <name>.in file, if
# there is one, is given to the program as its input.
#
#   tests/run-tests.sh            check everything
#   tests/run-tests.sh --update   write the .expected files from the current output
#
# Run it from anywhere. Needs a built compiler/elvm (make).

root=$(cd "$(dirname "$0")/.." && pwd)
elvm="$root/compiler/elvm"
update=0
passed=0
failed=0

[ "$1" = "--update" ] && update=1

if [ ! -x "$elvm" ]; then
  echo "compiler/elvm not found, run make first"
  exit 1
fi

for dir in "$root"/tests/*/; do
  cd "$dir" || exit 1

  for test in *.ell; do
    [ -f "$test" ] || continue
    name=${test%.ell}
    input=/dev/null

    [ -f "$name.in" ] && input="$name.in"

    "$elvm" "$test" < "$input" > "$name.actual" 2>&1
    echo "exit=$?" >> "$name.actual"

    if [ $update -eq 1 ]; then
      mv "$name.actual" "$name.expected"
    elif cmp -s "$name.actual" "$name.expected"; then
      passed=$((passed + 1))
      rm "$name.actual"
    else
      failed=$((failed + 1))
      echo "FAILED: $(basename "$dir")/$test (see $name.actual)"
    fi
  done
done

[ $update -eq 1 ] && echo "expected files written" && exit 0

echo "$passed passed, $failed failed"
[ $failed -eq 0 ]
