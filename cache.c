/*
 * cache.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "cache.h"
#include "main.h"
/* cache configuration parameters */
static int cache_split = 0;
static int cache_usize = DEFAULT_CACHE_SIZE;
static int cache_isize = DEFAULT_CACHE_SIZE; 
static int cache_dsize = DEFAULT_CACHE_SIZE;
static int cache_block_size = DEFAULT_CACHE_BLOCK_SIZE;
static int words_per_block = DEFAULT_CACHE_BLOCK_SIZE / WORD_SIZE;
static int cache_assoc = DEFAULT_CACHE_ASSOC;
static int cache_writeback = DEFAULT_CACHE_WRITEBACK;
static int cache_writealloc = DEFAULT_CACHE_WRITEALLOC;
/* cache model data structures */
static Pcache icache;
static Pcache dcache;
static cache c1;
static cache c2;
static cache mycache;
static cache_stat cache_stat_inst;
static cache_stat cache_stat_data;
/************************************************************/
void set_cache_param(param, value)
  int param;
  int value;
{
  switch (param) {
  case CACHE_PARAM_BLOCK_SIZE:
    cache_block_size = value;
    words_per_block = value / WORD_SIZE;
    break;
  case CACHE_PARAM_USIZE:
    cache_split = FALSE;
    cache_usize = value;
    break;
  case CACHE_PARAM_ISIZE:
    cache_split = TRUE;
    cache_isize = value;
    break;
  case CACHE_PARAM_DSIZE:
    cache_split = TRUE;
    cache_dsize = value;
    break;
  case CACHE_PARAM_ASSOC:
    cache_assoc = value;
    break;
  case CACHE_PARAM_WRITEBACK:
    cache_writeback = TRUE;
    break;
  case CACHE_PARAM_WRITETHROUGH:
    cache_writeback = FALSE;
    break;
  case CACHE_PARAM_WRITEALLOC:
    cache_writealloc = TRUE;
    break;
  case CACHE_PARAM_NOWRITEALLOC:
    cache_writealloc = FALSE;
    break;
  default:
    printf("error set_cache_param: bad parameter value\n");
    exit(-1);
  }

}

void init_cache()
{
    int i ;
      //checks if we want to use split cache 
      if (cache_split == 1)
      {
        icache = (Pcache)malloc(sizeof(Pcache));
        dcache = (Pcache)malloc(sizeof(Pcache));
        icache->size = cache_isize;
        dcache->size = cache_dsize;
        icache->n_sets = cache_isize / cache_block_size;
        dcache->n_sets = cache_dsize / cache_block_size;
        icache->associativity = cache_assoc;
        dcache->associativity = cache_assoc;
        icache->index_mask_offset = LOG2(cache_block_size);
        dcache->index_mask_offset = LOG2(cache_block_size);
        icache->index_mask = (1 << (LOG2(icache->n_sets)+icache->index_mask_offset)) - 1;
        dcache->index_mask = (1 << (LOG2(dcache->n_sets)+dcache->index_mask_offset)) - 1;
        icache->LRU_head=malloc(icache->n_sets*sizeof(cache_line));
        dcache->LRU_head=malloc(dcache->n_sets*sizeof(cache_line));
        for (i=1;i<icache->n_sets;i++)
        {
          icache->LRU_head[i].valid = 0;
          icache->LRU_head[i].dirty = 0;
          
        }
        for (i=1;i<dcache->n_sets;i++)
        {
          dcache->LRU_head[i].valid = 0;
          dcache->LRU_head[i].dirty = 0;
          
        }
      }
      else{
      mycache.size=cache_usize;
      mycache.n_sets=cache_usize/cache_block_size;
      mycache.associativity=cache_assoc;
      mycache.index_mask_offset=LOG2(cache_block_size);
      mycache.index_mask=(1 << (LOG2(mycache.n_sets)+mycache.index_mask_offset)) - 1;
      mycache.LRU_head=malloc(mycache.n_sets*sizeof(cache_line));
      for (i=1;i<mycache.n_sets;i++){
         mycache.LRU_head[i].valid=0;
         mycache.LRU_head[i].dirty=0;
         
      }
    }
  }
void perform_access(addr, access_type)
  unsigned addr, access_type;
{ // access type 1 , 0 = read and write / 2 = instruction
  unsigned _tag,_index;
  //Enter access split cache
  if (cache_split == 1)
  {
    //Start access to dcache
    if (access_type != 2)
    {
     _tag = addr>>(LOG2(dcache->n_sets) + dcache->index_mask_offset);
     _index = (addr&dcache->index_mask) >> dcache->index_mask_offset;
    if (dcache->LRU_head[_index].valid == 1)
    {
      if (dcache->LRU_head[_index].tag == _tag)
      {
        if (access_type == 1){
          dcache->LRU_head[_index].dirty = 1;
        }
        cache_stat_data.accesses ++;
      } 
      else{
         dcache->LRU_head[_index].tag = _tag;
         cache_stat_data.demand_fetches +=(cache_block_size/4);
        if (access_type == 1)
        {
         if (dcache->LRU_head[_index].dirty == 1)
         {
           dcache->LRU_head[_index].dirty = 0;
           cache_stat_data.copies_back += (cache_block_size/4);
         }
          cache_stat_data.replacements++;
          dcache->LRU_head[_index].dirty = 1;
	        cache_stat_data.accesses++;
	        cache_stat_data.misses++; 
        }
        else if(access_type == 0){
          if (dcache->LRU_head[_index].dirty == 1)
         {
           dcache->LRU_head[_index].dirty = 0;
           cache_stat_data.copies_back += (cache_block_size/4);
          }
          cache_stat_data.replacements++;
	        cache_stat_data.accesses++;
	        cache_stat_data.misses++; 
        }    
      }
    }
    //cold miss it means there is no data in cache 
    else{
      cache_stat_data.demand_fetches +=(cache_block_size/4);
      dcache->LRU_head[_index].tag = _tag;
      dcache->LRU_head[_index].valid = 1;
      if (access_type == 1)
      {
        dcache->LRU_head[_index].dirty = 1;
        } 
        cache_stat_data.accesses++;
        cache_stat_data.misses++;
    } 
  }
  //Start access the icache 
  else{
   _tag = addr>>(LOG2(icache->n_sets) +  icache->index_mask_offset);
   _index = (addr&icache->index_mask) >> icache->index_mask_offset;
   if (icache->LRU_head[_index].valid == 1)
   {
     if (icache->LRU_head[_index].tag == _tag)
     {
       cache_stat_inst.accesses++;
     }
     else{  
       icache->LRU_head[_index].tag = _tag;
       cache_stat_inst.replacements++;
       cache_stat_inst.accesses++;
       cache_stat_inst.misses++;
       cache_stat_inst.demand_fetches += (cache_block_size/4);
     }
   }
   else{
       icache->LRU_head[_index].valid = 1;
       icache->LRU_head[_index].tag = _tag;
       cache_stat_inst.accesses++;
       cache_stat_inst.misses++;
       cache_stat_inst.demand_fetches += (cache_block_size/4);
   }
  }
}
//Enter the Unified cache
else {
  _tag=addr>>(LOG2(mycache.n_sets)+mycache.index_mask_offset);
  _index=(addr&mycache.index_mask)>>mycache.index_mask_offset;
  if(mycache.LRU_head[_index].valid == 1){
     if(mycache.LRU_head[_index].tag == _tag){
        if(access_type == 1){
	   mycache.LRU_head[_index].dirty=1;
	   cache_stat_data.accesses++;
	}
	else if(access_type == 0){
	   cache_stat_data.accesses++;
	}
	else if(access_type == 2){
	   cache_stat_inst.accesses++;
	}
	else{
	}
     }
     else{
        mycache.LRU_head[_index].tag=_tag;
        cache_stat_data.demand_fetches += (cache_block_size/4);
        if(access_type == 1){
	   if(mycache.LRU_head[_index].dirty == 1){
	      cache_stat_data.copies_back+= (cache_block_size/4);
	      mycache.LRU_head[_index].dirty = 0;
	   }
     cache_stat_data.replacements++;
	   mycache.LRU_head[_index].dirty=1;
	   cache_stat_data.accesses++;
	   cache_stat_data.misses++;
	}
	else if(access_type == 0){
	   if(mycache.LRU_head[_index].dirty == 1){
	      cache_stat_data.copies_back+= (cache_block_size/4);
	      mycache.LRU_head[_index].dirty = 0;
	   }
     cache_stat_data.replacements++;
	   cache_stat_data.accesses++;
	   cache_stat_data.misses++;
	}
	else if(access_type == 2){
	   if(mycache.LRU_head[_index].dirty == 1){
	      cache_stat_inst.copies_back+= (cache_block_size/4);
	      mycache.LRU_head[_index].dirty = 0;
	   }
     cache_stat_inst.replacements++;
	   cache_stat_inst.accesses++;
	   cache_stat_inst.misses++;
	}
   }
 }
  else{
     cache_stat_data.demand_fetches += (cache_block_size/4);
     mycache.LRU_head[_index].tag=_tag;
     mycache.LRU_head[_index].valid=1;
     if(access_type == 1){
        mycache.LRU_head[_index].dirty=1;
        cache_stat_data.accesses++;
        cache_stat_data.misses++;
     }
     else if(access_type == 0){
        cache_stat_data.accesses++;
        cache_stat_data.misses++;
     }
     else if(access_type == 2){
        cache_stat_inst.accesses++;
        cache_stat_inst.misses++;
     }

    }
  }
 } 
void flush()
{
  int i;
  if (cache_split == 1)
  {
    for ( i = 0; i < dcache->n_sets; i++)
  {
    if (dcache->LRU_head[i].dirty == 1)
    {
      cache_stat_data.copies_back += (cache_block_size/4);
     } 
    }
  }
  else {
  for(i=0;i<mycache.n_sets;i++)
     if(mycache.LRU_head[i].dirty == 1)
	 cache_stat_data.copies_back += (cache_block_size/4);
 }
}
void dump_settings()
{
  printf("*** CACHE SETTINGS ***\n");
  if (cache_split) {
    printf("  Split cache\n");
    printf("  I-cache size: \t%d\n", cache_isize);
    printf("  D-cache size: \t%d\n", cache_dsize);
  } else {
    printf("  Cache type: \t\tUnified\n");
    printf("  Size: \t\t%d\n", cache_usize);
  }
  printf("  Associativity: \t%d\n", cache_assoc);
  printf("  Block size: \t\t%d\n", cache_block_size);
  printf("  Write policy: \t%s\n", 
	 cache_writeback ? "WRITE BACK" : "WRITE THROUGH");
  printf("  Allocation policy: \t%s\n",
	 cache_writealloc ? "WRITE ALLOCATE" : "WRITE NO ALLOCATE");
}

void print_stats()
{
  printf("\n*** CACHE STATISTICS ***\n");

  printf(" INSTRUCTIONS\n");
  printf("  accesses:  \t\t%d\n", cache_stat_inst.accesses);
  printf("  misses:    \t\t%d\n", cache_stat_inst.misses);
  if (!cache_stat_inst.accesses)
    printf("  miss rate: 0 (0)\n"); 
  else
    printf("  miss rate: \t\t%2.4f (hit rate %2.4f)\n", 
	 (float)cache_stat_inst.misses / (float)cache_stat_inst.accesses,
	 1.0 - (float)cache_stat_inst.misses / (float)cache_stat_inst.accesses);
  printf("  replace:   \t\t%d\n", cache_stat_inst.replacements);

  printf(" DATA\n");
  printf("  accesses:  \t\t%d\n", cache_stat_data.accesses);
  printf("  misses:    \t\t%d\n", cache_stat_data.misses);
  if (!cache_stat_data.accesses)
    printf("  miss rate: 0 (0)\n"); 
  else
    printf("  miss rate: \t\t%2.4f (hit rate %2.4f)\n", 
	 (float)cache_stat_data.misses / (float)cache_stat_data.accesses,
	 1.0 - (float)cache_stat_data.misses / (float)cache_stat_data.accesses);
  printf("  replace:   \t\t%d\n", cache_stat_data.replacements);

  printf(" TRAFFIC (in words)\n");
  printf("  demand fetch: \t%d\n", cache_stat_inst.demand_fetches + 
	 cache_stat_data.demand_fetches);
  printf("  copies back:  \t%d\n", cache_stat_inst.copies_back +
	 cache_stat_data.copies_back);
}