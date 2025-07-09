 #ifndef _MMU_H
#define _MMU_H

void mmu_init(void);

uint64_t mmu_get_l1_block(int i);

#endif