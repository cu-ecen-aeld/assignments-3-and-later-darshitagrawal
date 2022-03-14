/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /*Check for valid input parameters*/
    if((buffer == NULL) || (entry_offset_byte_rtn == NULL))
    {
        return NULL;
    }
    
    /*Check for empty buffer*/
    if((buffer->in_offs == buffer->out_offs) && (buffer->full == false))
    {
        return NULL;
    }
    
    size_t size = buffer->entry[buffer->out_offs].size;
    uint8_t copy_out_offs = buffer->out_offs;
    size_t temp_size = 0;
    
    for(int count = 0; count < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; count++)
    {
        if(char_offset > size - 1)
        {
            copy_out_offs++;
            if(copy_out_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
            {
                copy_out_offs = 0;
            }
            temp_size = size;
            size = size + buffer->entry[copy_out_offs].size;
        }
        
        else
        {
            *entry_offset_byte_rtn = char_offset - temp_size;
            return &buffer->entry[copy_out_offs];
        }
    }
    /**
    * TODO: implement per description
    */
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    const char* retval = NULL;
    /*Check for valid input parameters*/
    if((buffer == NULL) || (add_entry == NULL) || (add_entry.size == 0) || (add_entry.buffptr == NULL))
    {
        return retval;
    }
    

    /*Check for buffer full condition*/
    if(buffer->full)
    {
        retval = buffer->entry[buffer->out_offs].buffptr;
        buffer->out_offs = buffer->out_offs + 1;
        if(buffer->out_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        {
            buffer->out_offs = 0;
        }
    }
    
    buffer->entry[buffer->in_offs] = *add_entry;
    buffer->in_offs = buffer->in_offs + 1;
    
    if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        buffer->in_offs = 0;
    }
    if(buffer->in_offs == buffer->out_offs)
    {
        buffer->full = true;
    }
    return retval;
        
    /**
    * TODO: implement per description 
    */
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}

void aesd_circular_buffer_free(struct aesd_circular_buffer *buffer) 
{
    struct aesd_circular_buffer *buf = buffer;
    struct aesd_buffer_entry *buffer_entry;
    uint8_t index;

    AESD_CIRCULAR_BUFFER_FOREACH(entry,buf,index) 
    {
        if(entry->buffptr != NULL)
        {
            #ifdef __KERNEL__
            kfree((void*)entry->buffptr);
            #else
            free((void*)entry->buffptr);            
        }     
    }
}
