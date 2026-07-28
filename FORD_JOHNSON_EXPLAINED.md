# PmergeMe — Ford-Johnson (Merge-Insertion Sort) — Evaluation Notes

This document is written so you can defend the implementation in `PmergeMe.cpp`
during the CPP09 evaluation: what the algorithm is, why each design choice was
made, and why the comparison count you observe can be *at or below* the
classic Ford-Johnson worst-case table — not a fixed magic number.

---

## 1. The algorithm, on paper

Ford-Johnson (a.k.a. merge-insertion sort) sorts `n` elements while trying to
get as close as possible to the information-theoretic minimum number of
comparisons, `⌈log2(n!)⌉`. It beats a plain merge sort or insertion sort
because it inserts elements into the sorted chain using **bounded binary
search**, and it chooses the *order* of insertion (Jacobsthal order) so that
each binary search gets the largest possible search window without ever
exceeding the number of comparisons the window "deserves".

### Step by step

1. **Pair up the elements.** Split the `n` input elements into `⌊n/2⌋` pairs
   (with one leftover "straggler" if `n` is odd). Compare each pair once and
   put the larger element in `mainChain[i]`, the smaller in `pendChain[i]`.
   → this costs exactly `⌊n/2⌋` comparisons.

   ```
   input:  12 45 78 95 65 32 ...
   pairs:  (12,45) (78,95) (65,32) ...
   main:    45      95      65     ...
   pend:    12      78      32     ...
   ```

2. **Recursively sort the main chain.** The `mainChain` (the larger element
   of every pair) is itself sorted with the *same* Ford-Johnson algorithm.
   This is the recursive step — it's what makes the algorithm "merge" (it
   merges the recursive solution back together) rather than a flat
   insertion sort.

3. **Reorder the pend chain to match.** Once `mainChain` is sorted, each
   `pendChain[i]` is still tied to its original partner. We permute
   `pendChain`/its indices so that `pend[i]` corresponds to `mainChain[i]`
   *after* sorting — this is bookkeeping, not a comparison.

4. **Build the initial sorted chain.** The smallest guaranteed element is
   `pend[0]` (it lost its comparison against `main[0]`, and `main[0]` is now
   the smallest of the main chain), so the initial sorted sequence is:

   ```
   sorted = [ pend[0], main[0], main[1], main[2], ..., main[k-1] ]
   ```

   No comparisons needed here — this ordering is *known* from step 1/2.

5. **Insert the rest of the pend elements (and the straggler) in Jacobsthal
   order.** This is the clever part. Instead of inserting `pend[1], pend[2],
   pend[3], ...` in that order, we insert them in the order dictated by the
   Jacobsthal sequence: `1, 3, 5, 11, 21, 43, ...` (`J(k) = J(k-1) + 2·J(k-2)`),
   processed as descending groups. Concretely for the elements
   `pend[1..k]` we insert in the order:

   ```
   pend[3], pend[2]   (group ending at Jacobsthal(2)=3)
   pend[5], pend[4]   (group ending at Jacobsthal(3)=5)
   pend[11], pend[10], ..., pend[6]   (group ending at Jacobsthal(4)=11)
   ...
   ```

   **Why this order matters:** when we go to insert `pend[i]`, we already
   know it is smaller than `main[i]` (from step 1). Since `main[i]` sits at
   a *known* position in the sorted chain, we never need to binary-search
   past that position — we only need to search the prefix of `sorted` up to
   where `main[i]` lives. Jacobsthal order guarantees that by the time we
   insert `pend[i]`, the prefix in front of `main[i]`'s position has exactly
   `2^k - 1` elements in it for the smallest `k` that makes the search
   "worth" `k` comparisons. That is precisely what keeps every single
   insertion at the `⌈log2(window+1)⌉` comparison cost instead of a larger
   one.

6. **Binary search + insert.** Each pend element (and the straggler at the
   end) is located via `std::lower_bound` inside the bounded prefix
   described above, then inserted at that position (`O(n)` shift, not
   counted as a comparison).

### Concrete numbers for reference
The classical Ford-Johnson worst-case comparison counts (OEIS A001768),
for `n = 1..12`: `0, 1, 3, 5, 7, 10, 13, 16, 19, 22, 26, 30, ...`

For `n = 21` the worst-case bound is **66**; for `n = 22` it is **71**. These
are *upper bounds that the algorithm never exceeds* — not values it always
hits exactly. See §3 for why your run can print `70` or lower for `n = 22`.

---

## 2. How the code implements it (mapping to `PmergeMe.cpp`)

| Concept | Code |
|---|---|
| Pairing + first comparison | `fjSortVec` lines ~121-134, `++comps` inside the pairing loop |
| Recursive sort of main chain | `fjSortVec(mainChain, mainPerm, comps);` (line 137) — recursion, not a library sort |
| Re-associating pend with sorted main | the `sp[i] = pendChain[mainPerm[i]]` loop (lines 141-146) |
| Building the initial chain `[pend0, main0..mainK]` | lines 148-157 |
| Jacobsthal insertion order | `jacobsthalOrder<IdxC>(n)` (lines 56-95), templated so both the `vector` and `deque` paths reuse identical logic |
| Bounded binary search | `pendAllPos[idx]` tracks *where `main[i]` currently sits* in `sorted`; `sorted.begin() + boundPos` caps the search window (lines 186-191) |
| Comparison counting | `CountComp` functor passed to `std::lower_bound` (line 191/300) plus manual `++comps` for the pairing phase |
| Index bookkeeping | every chain (`mainIdx`, `pendIdx`, `sortedOrig`, `pendAllPos`) exists purely to preserve *original array positions* so the final `perm`/`sortedOrig` can be discarded — the algorithm itself only cares about relative order, but tracking original indices makes the bounding trick in step 5 possible without re-deriving positions each time |
| Straggler (odd `n`) | appended manually as the last "pend" element, given `pendAllPos = -1` (i.e. "no upper bound, search the whole current chain") since it has no paired `main` element to bound it (lines 173-178, 296-297) |
| Two containers, same algorithm | `fjSortVec`/`fjSortDeq` are line-for-line identical logic on `std::vector`/`std::deque` — required by the subject (must show the algorithm working on two different container types) |

### Design choices worth explaining verbally in the eval

- **Why recursion instead of an iterative main-chain sort?** Merge-insertion
  is defined recursively — `T(n) = T(n/2) + insertions`. Recursing is the
  natural and provably-optimal-comparison-count way to sort the main chain;
  re-sorting it with `std::sort` would use an unrelated (and non-Ford-Johnson)
  comparison count, defeating the point of the exercise.

- **Why track original indices (`mainIdx`, `pendIdx`, `sortedOrig`) at all?**
  We're not sorting a plain `int` array — we're recursively rearranging
  chains, so we need a permutation to translate the recursive sort's answer
  back to the *pend* chain's order (step 3), and to know exactly where each
  pend's paired `main` element currently sits so we can bound the binary
  search (step 5). Losing the index would force an unbounded binary search
  over the whole array — still correct, just not comparison-optimal.

- **Why `std::lower_bound` instead of a hand-rolled binary search?** Same
  algorithm, but it's the standard, well-tested STL binary search — and it
  accepts a custom comparator (`CountComp`) so every comparison it performs
  is still counted, which the subject requires ("count and display the
  comparisons").

---

## 3. Why the comparison count is sometimes *below* the Ford-Johnson table value

This is the part worth being precise about in the eval, because "Ford-Johnson
gives 66 (or 71, or whatever the table says)" is describing a **worst-case
upper bound**, not a constant the algorithm always produces.

1. **The table value is an adversarial worst case.** It's derived assuming
   every binary search needs its full `⌈log2(window+1)⌉` comparisons, i.e.
   the element being inserted is always at the "hardest" position in the
   window. For arbitrary input, `std::lower_bound` can resolve in *fewer*
   comparisons — e.g. if the value being inserted happens to land near an
   edge of the search window, some binary search steps short-circuit sooner
   than the theoretical worst path.

2. **The bounding trick itself can shrink windows below the formula's
   assumption.** The classical formula assumes the "ideal" window sizes from
   the Jacobsthal grouping. In this implementation, `boundPos` is derived
   from the *live* position of the paired `main` element (`pos[j]` is kept
   updated after every insertion, lines 197-201/306-310). If earlier
   insertions shifted that boundary earlier than the theoretical ideal, the
   actual search window handed to `std::lower_bound` can be smaller than the
   formula's worst-case window — meaning fewer comparisons are structurally
   possible for that element, not just luck.

3. **Duplicate values.** With duplicate input values (as in your test case:
   `45` appears twice, `54` twice, `65` twice, `87` twice, `32` twice, `51`
   twice), a duplicate can match a boundary value in fewer probe steps than a
   strictly-ordered adversarial sequence would need, shaving comparisons off
   the totals in the pairing phase and in some binary searches.

4. **It is never *above* the table value.** The bounding + Jacobsthal-order
   combination is exactly what guarantees the count never exceeds the known
   Ford-Johnson upper bound for that `n` — that's the entire point of the
   algorithm. So what you should expect to say in the eval is:

   > "My comparison count is always ≤ the Ford-Johnson worst-case bound for
   > this `n`, and it can be strictly less on inputs that aren't adversarial
   > for the binary search — for example when duplicate values or a
   > favorable pairing let `std::lower_bound` terminate early."

If you want to *demonstrate* the exact worst-case number for a given `n`,
run the program on a maximally adversarial permutation (a classic
"worst-case for merge-insertion" input — such an input can be constructed but
isn't a value you need to know off-hand). Getting a lower count on ordinary
inputs is expected behavior, not a bug.

---

## 4. Quick answers for likely eval questions

- **"Why merge-insertion and not quicksort/mergesort?"** — subject requires
  Ford-Johnson specifically; it's chosen for its near-optimal comparison
  count, which is the metric the subject asks you to display.
- **"What happens with `n` odd?"** — the leftover element becomes the
  `straggler`, appended to the pend list with no upper bound on its
  insertion search (it has no paired `main` element to bound it).
- **"What happens with `n <= 1`?"** — `fjSortVec`/`fjSortDeq` return
  immediately (line 112/222); a 0- or 1-element chain is already sorted by
  definition, 0 comparisons.
- **"Why two containers (vector/deque)?"** — subject requirement, to compare
  performance/complexity characteristics of contiguous vs. node-based
  storage on the same algorithm; the timing output isolates this.
- **"Why is `0` accepted as valid input now?"** — the subject's "positive
  integer" wording was interpreted to include `0` per your instruction; the
  parser's check was changed from `num <= 0` to `num < 0`
  (`PmergeMe.cpp:42`).
