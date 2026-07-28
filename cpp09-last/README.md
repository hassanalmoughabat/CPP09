# CPP Module 09 — STL

**Author:** hal-moug (42)

C++98, compiled with `c++ -Wall -Wextra -Werror -std=c++98`.

Module rule: **once a container is used, it cannot be reused in the module.**

| Exercise | Program | Container(s) used |
|----------|---------|-------------------|
| ex00 | `btc` — Bitcoin Exchange | `std::map` |
| ex01 | `RPN` — Reverse Polish Notation | `std::stack` |
| ex02 | `PmergeMe` — merge-insert sort | `std::vector` + `std::deque` |

---

# ex02 — PmergeMe

Sorts a sequence of positive integers with the **Ford-Johnson (merge-insertion) algorithm**
(Knuth, *The Art of Computer Programming*, Vol. 3, p. 184), implemented **separately** for
`std::vector` and `std::deque` as the subject advises, and prints the time each container took.

## Build & run

```bash
cd ex02
make
./PmergeMe 3 5 9 7 4
```

```
Before: 3 5 9 7 4
After: 3 4 5 7 9
Time to process a range of 5 elements with std::vector : 11 us
Time to process a range of 5 elements with std::deque  : 21 us
Counting the vector comparisons: 7
Counting the deque  comparisons: 7
```

The two `Counting ...` lines are extra (allowed — the subject fixes only the first four lines).
They exist to prove during the defense that the comparison count matches the Ford-Johnson
theoretical bound (see below).

## The algorithm, step by step

1. **Pair & compare** — split the sequence into ⌊n/2⌋ pairs; one comparison per pair puts the
   larger element in the *main chain* and the smaller in the *pend chain*. An odd leftover
   element becomes the *straggler*.
2. **Recurse** — sort the main chain recursively with the same algorithm (the recursion also
   returns the permutation, so each pend element stays attached to its partner).
3. **Insert** — the pend element paired with the smallest main-chain element is prepended for
   free (it is smaller than its partner by construction). The remaining pend elements and the
   straggler are inserted by **binary search**, in the order given by the **Jacobsthal
   sequence** (1, 3, 5, 11, 21, 43, ...), from each group's highest index down to its lowest.
4. **Bounded search** — each pend element `b_k` is only searched among the elements *before its
   partner* `a_k` (it is already known to be smaller than `a_k`). The Jacobsthal order makes
   every search area contain exactly `2^m − 1` candidates, so each binary search costs exactly
   `m` comparisons in the worst case — this is what makes Ford-Johnson comparison-optimal.

Why Jacobsthal? Inserting group k (indices between two consecutive Jacobsthal numbers) from the
top down keeps the binary-search window at the worst-case-optimal size `2^m − 1` for every
insertion in the group, instead of letting the window grow past a power-of-two boundary.

## Why `vector` and `deque`?

Binary-search insertion needs **random-access iterators** (`std::lower_bound` + positional
`insert`), which rules out `std::list`. `std::set`/`std::map` would do the sorting themselves,
which defeats the exercise.

* `std::vector` — one contiguous block: best cache locality, cheapest random access, a middle
  insert is a single fast element shift.
* `std::deque` — same interface, radically different layout: fixed-size chunks addressed
  through a map of pointers. Every access pays an extra indirection and middle inserts touch
  multiple chunks.

Same algorithm + same comparison count, different memory layout — which is exactly why the
subject wants two containers timed:

| n | vector | deque |
|---|--------|-------|
| 1 000 | ~1.6 ms | ~16 ms |
| 3 000 | ~15 ms | ~148 ms |

The displayed time covers **data management (loading the container) + sorting**, as required.

## Error handling

Anything invalid prints `Error` on **stderr** and exits with status 1.

| Input | Result | Why |
|-------|--------|-----|
| *(no arguments)* | `Error` | nothing to sort |
| `-1`, `0` | `Error` | only **positive** integers accepted |
| `2147483648`, `4294967295`, 26-digit numbers | `Error` | > INT_MAX |
| `abc`, `42abc`, `1.5`, `1e5`, `0x10`, `+42`, `""` | `Error` | digits only |
| `"1 2 3"` (one quoted argument) | `Error` | a space is not a digit |
| `007` | accepted as `7` | leading zeros are still digits |
| `5 5 3 3 1` | sorted normally | duplicate handling is left to our discretion (subject) |
| `2147483647` | accepted | INT_MAX itself is valid |

## Hard tests — everything below must pass

### 1. Eval-sheet commands (Linux)

```bash
# 5–10 different positive integers → sorted output
./PmergeMe 8 3 99 42 7 15 1

# the exact eval command (shuf without -r caps at 1000 unique numbers — that's expected)
./PmergeMe `shuf -i 1-1000 -n 3000 | tr "\n" " "`

# the subject's 3000-different-integers command
./PmergeMe `shuf -i 1-100000 -n 3000 | tr "\n" " "`

# error case from the subject
./PmergeMe "-1" "2"        # -> Error (on stderr, exit 1)
```

Verify the output is really sorted without reading 3000 numbers:

```bash
./PmergeMe `shuf -i 1-100000 -n 3000 | tr "\n" " "` \
  | grep '^After:' | tr ' ' '\n' | tail -n +2 | sort -nc && echo SORTED-OK
```

> Shell note: if you store the numbers in a variable, `zsh` does **not** word-split `$args` —
> the program then receives one giant argument and correctly prints `Error`. Use backticks as
> above, or `${=args}` in zsh. This is shell behavior, not a program bug.

### 2. Correctness vs `sort -n` (stress)

Every size 1–100, duplicates, already-sorted, reverse-sorted, all-equal, and 500/1000/3000/10000
random elements — the `After:` line must equal `sort -n` of the input. Ran 117/117 + 30 repeat
rounds of the eval command: all matched.

```bash
in=$(shuf -i 1-100000 -n 3000 | tr '\n' ' ')
diff <(./PmergeMe `echo $in` | grep '^After:' | tr ' ' '\n' | tail -n +2) \
     <(echo $in | tr ' ' '\n' | sort -n) && echo MATCH
```

### 3. Exhaustive permutations

All **46,233 permutations** of 1..n for every n ≤ 8, both containers verified independently
(the normal output only prints the vector result — the deque result was checked through a test
harness). 0 failures.

### 4. Ford-Johnson comparison bound (proof it's the real algorithm)

Merge-insertion's worst case is `F(n) = Σ ⌈log₂(3k/4)⌉` for k = 1..n. The classic evaluator
check: **21 elements must never need more than F(21) = 66 comparisons.**

```bash
for i in $(seq 1 50); do
  ./PmergeMe `shuf -i 1-1000000 -n 21 | tr "\n" " "` | grep vector.comparisons
done   # every value must be <= 66
```

Measured: worst case over 2000 random runs = **exactly 66, never more** (most runs land in the
low 60s — the bound is a maximum, not a constant). The F(n) bound was also verified for every
n ≤ 600. Exceeding it even once would mean the implementation is not true Ford-Johnson.

### 5. Memory & robustness

```bash
valgrind --leak-check=full ./PmergeMe `shuf -i 1-1000 -n 3000 | tr "\n" " "`
# in use at exit: 0 bytes in 0 blocks — 0 errors
```

* 100,000 elements: sorts correctly (deque is slow at that size — expected, requirement is 3000).
* No segfault or uncontrolled exit on any input above.
* `make` twice in a row → `Nothing to be done for 'all'` (no relink).

## Defense Q&A cheat sheet

* **Why is deque ~10× slower with the same number of comparisons?** The comparison counters
  print identical values, so the difference is pure memory layout: vector is contiguous
  (cache-friendly, inserts are one element shift), deque is chunked with pointer indirection on
  every access and multi-chunk middle inserts.
* **Why not a generic template for both containers?** The subject strongly advises one
  implementation per container; each path here stays inside its own container family.
* **Where is the Jacobsthal sequence?** `jacobsthalOrder()` in `PmergeMe.cpp` builds the
  insertion order; the bounded binary search is the `lower_bound(begin, bound, ...)` call where
  `bound` is the partner's tracked position.
* **Time measured?** `clock()` around container loading + sort, per container, printed in µs.
