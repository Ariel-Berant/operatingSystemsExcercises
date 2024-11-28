#include "os.h"
#include "stdio.h"

#define PT_ENTRY_LENGTH = 512

uint64_t getPtAddress(uint64_t *arr, uint64_t addr){
    uint64_t isValid = 1;
    isValid = isValid & arr[addr];
    if (!isValid)
    {
        arr[addr] = ((alloc_page_frame() << 12) | 1);
    }
    return arr[addr] >> 12;
}

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn){
    uint64_t *ptAddr = phys_to_virt(pt << 12);
    uint64_t currPhysAddrInd, tokenInd = 0x1FF000000000; // hexadecimal value of bits 45-37 being all one

    for (int i = 4; i > 0; i--)
    {
        currPhysAddrInd = vpn & tokenInd; // gets current token value
        currPhysAddrInd = currPhysAddrInd >> (9 * i); // sets token to beginning of uint
        tokenInd = tokenInd >> 9; // moves index for next time

        // gets frame number(if not valid creates one) and converts it to virtual address for us
        ptAddr = phys_to_virt(getPtAddress(ptAddr, currPhysAddrInd) << 12); 
    }
    
    if (ppn == NO_MAPPING)
    {
        ptAddr[vpn & tokenInd] = ptAddr[vpn & tokenInd] & (~1);
    }
    else
    {
        ptAddr[vpn & tokenInd] = (ppn << 12) | 1;
    }
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn){
    uint64_t *ptAddr = phys_to_virt(pt << 12);
    uint64_t currPhysAddrInd, tokenInd = 0x1FF000000000; // hexadecimal value of bits 45-37 being all one

    for (int i = 4; i > 0; i--)
    {
        currPhysAddrInd = vpn & tokenInd; // gets current token value
        currPhysAddrInd = currPhysAddrInd >> (9 * i); // sets token to beginning of uint
        tokenInd = tokenInd >> 9; // moves index for next time

        // checks if valid, if not returns NO_MAPPING, otherwise proceeds
        if (!(ptAddr[currPhysAddrInd] & 1)){
            return NO_MAPPING;
        }
        // we know next token is valid, so we move to next PT(won't create a new one - already valid)
        ptAddr = phys_to_virt(getPtAddress(ptAddr, currPhysAddrInd) << 12);
    }

    if (!(ptAddr[(vpn & tokenInd)] & 1))
    {
        return NO_MAPPING;
    }
    
    return (ptAddr[(vpn & tokenInd)] >> 12);
}
