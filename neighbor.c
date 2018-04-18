#include <assert.h>
#include "hkpriv.h"
#include "ksort.h"

#define nei_lt(a, b) ((a).d < (b).d)
KSORT_INIT(nei, struct hk_nei1, nei_lt)

static inline void nei_add(struct hk_nei *n, int max_nei, int i, int j, int d)
{
	struct hk_nei1 *n1 = &n->nei[n->offcnt[i] >> 16];
	int32_t c0 = n->offcnt[i]&0xffff;
	if (c0 < max_nei) {
		n1[c0].d = d;
		n1[c0].i = j;
		++n->offcnt[i];
		ks_heapup_nei(c0 + 1, n1);
	} else if (n1->d > d) {
		n1->d = d, n1->i = j;
		ks_heapdown_nei(0, c0, n1);
	}
}

struct hk_nei *hk_pair2nei(int n_pairs, const struct hk_pair *pairs, int max_radius, int max_nei)
{
	int32_t i;
	int64_t offset;
	struct hk_nei *n;

	assert(max_nei < 0x10000);
	n = CALLOC(struct hk_nei, 1);
	n->n_pairs = n_pairs;

	n->offcnt = CALLOC(uint64_t, n_pairs);
	for (i = 1; i < n_pairs; ++i) {
		const struct hk_pair *q = &pairs[i];
		int32_t j, q1 = hk_ppos1(q), q2 = hk_ppos2(q);
		for (j = i - 1; j >= 0; --j) {
			const struct hk_pair *p = &pairs[j];
			int32_t p1 = hk_ppos1(p), p2 = hk_ppos2(p), y, z;
			y = q1 - p1;
			z = q2 > p2? q2 - p2 : p2 - q2;
			if (y > max_radius) break;
			if (z > max_radius) continue;
			++n->offcnt[j];
			++n->offcnt[i];
			if (n->offcnt[i] > max_nei) break;
		}
	}
	for (i = 0, offset = 0; i < n_pairs; ++i) {
		int32_t c = n->offcnt[i] < max_nei? n->offcnt[i] : max_nei;
		n->offcnt[i] = offset << 16;
		offset += c;
	}
	n->nei = CALLOC(struct hk_nei1, offset);

	for (i = 1; i < n_pairs; ++i) {
		const struct hk_pair *q = &pairs[i];
		int32_t j, q1 = hk_ppos1(q), q2 = hk_ppos2(q);
		for (j = i - 1; j >= 0; --j) {
			const struct hk_pair *p = &pairs[j];
			int32_t p1 = hk_ppos1(p), p2 = hk_ppos2(p), y, z, d;
			y = q1 - p1;
			z = q2 > p2? q2 - p2 : p2 - q2;
			if (y > max_radius) break;
			if ((n->offcnt[i]&0xffff) == max_nei && y > n->nei[n->offcnt[i]>>16].d)
				break;
			if (z > max_radius) continue;
			d = y > z? y : z;
			nei_add(n, max_nei, i, j, d);
			nei_add(n, max_nei, j, i, d);
		}
	}
	for (i = 0; i < n_pairs; ++i)
		ks_heapsort_nei(n->offcnt[i]&0xffff, &n->nei[n->offcnt[i]>>16]);
	return n;
}
