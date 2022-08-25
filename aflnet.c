#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "alloc-inl.h"
#include "aflnet.h"

// Protocol-specific functions for extracting requests and responses

region_t* extract_requests_smtp(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref)
{
   char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int region_count = 0;
  region_t *regions = NULL;
  char terminator[2] = {0x0D, 0x0A};

  mem=(char *)ck_alloc(mem_size);

  unsigned int cur_start = 0;
  unsigned int cur_end = 0;
  while (byte_count < buf_size) {

    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the last two bytes are 0x0D0A
    if ((mem_count > 1) && (memcmp(&mem[mem_count - 1], terminator, 2) == 0)) {
      region_count++;
      regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
      regions[region_count - 1].start_byte = cur_start;
      regions[region_count - 1].end_byte = cur_end;
      regions[region_count - 1].state_sequence = NULL;
      regions[region_count - 1].state_count = 0;

      mem_count = 0;
      cur_start = cur_end + 1;
      cur_end = cur_start;
    } else {
      mem_count++;
      cur_end++;

      //Check if the last byte has been reached
      if (cur_end == buf_size - 1) {
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;
        break;
      }

      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);

  //in case region_count equals zero, it means that the structure of the buffer is broken
  //hence we create one region for the whole buffer
  if ((region_count == 0) && (buf_size > 0)) {
    regions = (region_t *)ck_realloc(regions, sizeof(region_t));
    regions[0].start_byte = 0;
    regions[0].end_byte = buf_size - 1;
    regions[0].state_sequence = NULL;
    regions[0].state_count = 0;

    region_count = 1;
  }

  *region_count_ref = region_count;
  return regions;
}

region_t* extract_requests_ssh(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int region_count = 0;
  region_t *regions = NULL;
  char terminator[2] = {0x0D, 0x0A};

  mem=(char *)ck_alloc(mem_size);

  unsigned int cur_start = 0;
  unsigned int cur_end = 0;
  while (byte_count < buf_size) {

    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the region buffer length is at least 6 bytes
    //Why 6 bytes? It is because both the SSH identification and the normal message are longer than 6 bytes
    //For normal message, it starts with message size (4 bytes), #padding_bytes (1 byte) and message code (1 byte)
    if (mem_count >= 6) {
      if (!strncmp(mem, "SSH-", 4)) {
        //It could be an identification message
        //Find terminator (0x0D 0x0A)
        while ((byte_count < buf_size) && (memcmp(&mem[mem_count - 1], terminator, 2))) {
          if (mem_count == mem_size - 1) {
            //enlarge the mem buffer
            mem_size = mem_size * 2;
            mem=(char *)ck_realloc(mem, mem_size);
          }
          memcpy(&mem[++mem_count], buf + byte_count++, 1);
          cur_end++;
        }

        //Create one region
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;

        //Check if the last byte has been reached
        if (cur_end < buf_size - 1) {
          mem_count = 0;
          cur_start = cur_end + 1;
          cur_end = cur_start;
        }
      } else {
        //It could be a normal message
        //Extract the message size stored in the first 4 bytes
        unsigned int* size_buf = (unsigned int*)&mem[0];
        unsigned int message_size = (unsigned int)ntohl(*size_buf);
        unsigned char message_code = (unsigned char)mem[5];
        //and skip the payload and the MAC
        unsigned int bytes_to_skip = message_size - 2;
        if ((message_code >= 20) && (message_code <= 49)) {
          //Do nothing
        } else {
          bytes_to_skip += 8;
        }

        unsigned int temp_count = 0;
        while ((byte_count < buf_size) && (temp_count < bytes_to_skip)) {
          byte_count++;
          cur_end++;
          temp_count++;
        }

        if (byte_count < buf_size) {
          byte_count--;
          cur_end--;
        }

        //Create one region
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;

        //Check if the last byte has been reached
        if (cur_end < buf_size - 1) {
          mem_count = 0;
          cur_start = cur_end + 1;
          cur_end = cur_start;
        }
      }
    } else {
      mem_count++;
      cur_end++;

      //Check if the last byte has been reached
      if (cur_end == buf_size - 1) {
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;
        break;
      }

      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);

  //in case region_count equals zero, it means that the structure of the buffer is broken
  //hence we create one region for the whole buffer
  if ((region_count == 0) && (buf_size > 0)) {
    regions = (region_t *)ck_realloc(regions, sizeof(region_t));
    regions[0].start_byte = 0;
    regions[0].end_byte = buf_size - 1;
    regions[0].state_sequence = NULL;
    regions[0].state_count = 0;

    region_count = 1;
  }

  *region_count_ref = region_count;
  return regions;
}

region_t* extract_requests_tls(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int region_count = 0;
  region_t *regions = NULL;

  mem=(char *)ck_alloc(mem_size);

  unsigned int cur_start = 0;
  unsigned int cur_end = 0;
  while (byte_count < buf_size) {

    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the region buffer length is at least 5 bytes (record header size)
    if (mem_count >= 5) {
      //1st byte: content type
      //2nd and 3rd byte: TLS version
      //Extract the message size stored in the 4th and 5th bytes
      u16* size_buf = (u16*)&mem[3];
      u16 message_size = (u16)ntohs(*size_buf);

      //and skip the payload
      unsigned int bytes_to_skip = message_size;

      unsigned int temp_count = 0;
      while ((byte_count < buf_size) && (temp_count < bytes_to_skip)) {
        byte_count++;
        cur_end++;
        temp_count++;
      }

      if (byte_count < buf_size) {
          byte_count--;
          cur_end--;
      }

      //Create one region
      region_count++;
      regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
      regions[region_count - 1].start_byte = cur_start;
      regions[region_count - 1].end_byte = cur_end;
      regions[region_count - 1].state_sequence = NULL;
      regions[region_count - 1].state_count = 0;

      //Check if the last byte has been reached
      if (cur_end < buf_size - 1) {
        mem_count = 0;
        cur_start = cur_end + 1;
        cur_end = cur_start;
      }
    } else {
      mem_count++;
      cur_end++;

      //Check if the last byte has been reached
      if (cur_end == buf_size - 1) {
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;
        break;
      }

      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);

  //in case region_count equals zero, it means that the structure of the buffer is broken
  //hence we create one region for the whole buffer
  if ((region_count == 0) && (buf_size > 0)) {
    regions = (region_t *)ck_realloc(regions, sizeof(region_t));
    regions[0].start_byte = 0;
    regions[0].end_byte = buf_size - 1;
    regions[0].state_sequence = NULL;
    regions[0].state_count = 0;

    region_count = 1;
  }

  *region_count_ref = region_count;
  return regions;
}


region_t* extract_requests_dicom(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref)
{
  unsigned int pdu_length = 0;
  unsigned int packet_length = 0;
  unsigned int region_count = 0;
  unsigned int end = 0;
  unsigned int start = 0;

  region_t *regions = NULL;

  unsigned int byte_count = 0;
  while (byte_count < buf_size) {

    if ((byte_count + 2 >= buf_size) || (byte_count + 5 >= buf_size)) break;

    // Bytes from third to sixth encode the PDU length.
    pdu_length =
      (buf[byte_count + 5]) |
      (buf[byte_count + 4] << 8)  |
      (buf[byte_count + 3] << 16) |
      (buf[byte_count + 2] << 24);

    // DICOM Header(6 bytes) includes PDU type and PDU length.
    packet_length = pdu_length + 6;

    start = byte_count;
    end = byte_count + packet_length - 1;

    if (end < start) break; // it means that int overflow has happened -_0_0_-
    if (end >= buf_size) break; // checking boundaries

    region_count++;
    regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
    regions[region_count - 1].start_byte = start;
    regions[region_count - 1].end_byte = end;
    regions[region_count - 1].state_sequence = NULL;
    regions[region_count - 1].state_count = 0;

    if ( (byte_count + packet_length) < byte_count ) break; // checking int overflow
    if ( (byte_count + packet_length) < packet_length ) break; // checking int overflow
    byte_count += packet_length;
  }

  // if bytes is left
  if ((byte_count < buf_size) && (buf_size > 0)) {
    region_count++;
    regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
    regions[region_count - 1].start_byte = byte_count;
    regions[region_count - 1].end_byte = buf_size - 1;
    regions[region_count - 1].state_sequence = NULL;
    regions[region_count - 1].state_count = 0;
  }

  *region_count_ref = region_count;
  return regions;
}

region_t* extract_requests_dns(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref)
{
  char *mem;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int region_count = 0;
  region_t *regions = NULL;

  mem = (char *)ck_alloc(mem_size);

  unsigned int cur_start = 0;
  unsigned int cur_end = 0;
  for (unsigned int byte_count = 0; byte_count < buf_size; byte_count++) {
    memcpy(&mem[mem_count], buf + byte_count, 1);

    // A DNS header is 12 bytes long & the 1st null byte after that indicates the end of the query.
    if ((mem_count >= 12) && (*(mem+mem_count) == 0)) {
      // 4 bytes left of the tail.
      cur_end += 4;
      byte_count += 4;
      region_count++;
      regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
      regions[region_count - 1].start_byte = cur_start;
      regions[region_count - 1].end_byte = cur_end;
      regions[region_count - 1].state_sequence = NULL;
      regions[region_count - 1].state_count = 0;

      if (cur_end == buf_size - 1) break;

      mem_count = 0;
      cur_start = cur_end + 1;
      cur_end = cur_start;
    } else {
      mem_count++;
      cur_end++;

      // Check if the last byte has been reached
      if (cur_end == buf_size - 1) {
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;
        break;
      }

      if (mem_count == mem_size) {
        // Enlarge the mem buffer
        mem_size *= 2;
        mem = (char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);

  // In case region_count equals zero, it means that the structure of the buffer is broken
  // hence we create one region for the whole buffer
  if ((region_count == 0) && (buf_size > 0)) {
    regions = (region_t *)ck_realloc(regions, sizeof(region_t));
    regions[0].start_byte = 0;
    regions[0].end_byte = buf_size - 1;
    regions[0].state_sequence = NULL;
    regions[0].state_count = 0;

    region_count = 1;
  }

  *region_count_ref = region_count;
  return regions;
}

region_t* extract_requests_rtsp(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int region_count = 0;
  region_t *regions = NULL;
  char terminator[4] = {0x0D, 0x0A, 0x0D, 0x0A};

  mem=(char *)ck_alloc(mem_size);

  unsigned int cur_start = 0;
  unsigned int cur_end = 0;
  while (byte_count < buf_size) {

    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the last four bytes are 0x0D0A0D0A
    if ((mem_count > 3) && (memcmp(&mem[mem_count - 3], terminator, 4) == 0)) {
      region_count++;
      regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
      regions[region_count - 1].start_byte = cur_start;
      regions[region_count - 1].end_byte = cur_end;
      regions[region_count - 1].state_sequence = NULL;
      regions[region_count - 1].state_count = 0;

      mem_count = 0;
      cur_start = cur_end + 1;
      cur_end = cur_start;
    } else {
      mem_count++;
      cur_end++;

      //Check if the last byte has been reached
      if (cur_end == buf_size - 1) {
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;
        break;
      }

      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);

  //in case region_count equals zero, it means that the structure of the buffer is broken
  //hence we create one region for the whole buffer
  if ((region_count == 0) && (buf_size > 0)) {
    regions = (region_t *)ck_realloc(regions, sizeof(region_t));
    regions[0].start_byte = 0;
    regions[0].end_byte = buf_size - 1;
    regions[0].state_sequence = NULL;
    regions[0].state_count = 0;

    region_count = 1;
  }

  *region_count_ref = region_count;
  return regions;
}

region_t* extract_requests_ftp(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int region_count = 0;
  region_t *regions = NULL;
  char terminator[2] = {0x0D, 0x0A};

  mem=(char *)ck_alloc(mem_size);

  unsigned int cur_start = 0;
  unsigned int cur_end = 0;
  while (byte_count < buf_size) {

    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the last two bytes are 0x0D0A
    if ((mem_count > 1) && (memcmp(&mem[mem_count - 1], terminator, 2) == 0)) {
      region_count++;
      regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
      regions[region_count - 1].start_byte = cur_start;
      regions[region_count - 1].end_byte = cur_end;
      regions[region_count - 1].state_sequence = NULL;
      regions[region_count - 1].state_count = 0;

      mem_count = 0;
      cur_start = cur_end + 1;
      cur_end = cur_start;
    } else {
      mem_count++;
      cur_end++;

      //Check if the last byte has been reached
      if (cur_end == buf_size - 1) {
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;
        break;
      }

      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);

  //in case region_count equals zero, it means that the structure of the buffer is broken
  //hence we create one region for the whole buffer
  if ((region_count == 0) && (buf_size > 0)) {
    regions = (region_t *)ck_realloc(regions, sizeof(region_t));
    regions[0].start_byte = 0;
    regions[0].end_byte = buf_size - 1;
    regions[0].state_sequence = NULL;
    regions[0].state_count = 0;

    region_count = 1;
  }

  *region_count_ref = region_count;
  return regions;
}

region_t* extract_requests_sip(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int region_count = 0;
  region_t *regions = NULL;
  char terminator[2] = {0x0D, 0x0A};

  mem=(char *)ck_alloc(mem_size);

  unsigned int cur_start = 0;
  unsigned int cur_end = 0;
  while (byte_count < buf_size) {

    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the last bytes match the terminator, and the next ones are a SIP command
    if ((mem_count > 1) && (memcmp(&mem[mem_count - 1], terminator, 1) == 0) &&
	  (((buf_size - byte_count >= 8) && (memcmp(buf + byte_count, "REGISTER", 8)==0) ) ||
	  ((buf_size - byte_count >= 6) && (memcmp(buf + byte_count, "INVITE", 6)==0) ) ||
	  ((buf_size - byte_count >= 3) && (memcmp(buf + byte_count, "ACK", 3)==0) ) ||
	  ((buf_size - byte_count >= 3) && (memcmp(buf + byte_count, "BYE", 3)==0) ) )
	  ) {
      region_count++;
      regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
      regions[region_count - 1].start_byte = cur_start;
      regions[region_count - 1].end_byte = cur_end;
      regions[region_count - 1].state_sequence = NULL;
      regions[region_count - 1].state_count = 0;

      mem_count = 0;
      cur_start = cur_end + 1;
      cur_end = cur_start;
    } else {
      mem_count++;
      cur_end++;

      //Check if the last byte has been reached
      if (cur_end == buf_size - 1) {
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;
        break;
      }

      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);

  //in case region_count equals zero, it means that the structure of the buffer is broken
  //hence we create one region for the whole buffer
  if ((region_count == 0) && (buf_size > 0)) {
    regions = (region_t *)ck_realloc(regions, sizeof(region_t));
    regions[0].start_byte = 0;
    regions[0].end_byte = buf_size - 1;
    regions[0].state_sequence = NULL;
    regions[0].state_count = 0;

    region_count = 1;
  }

  *region_count_ref = region_count;
  return regions;
}

region_t* extract_requests_http(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int region_count = 0;
  region_t *regions = NULL;
  char terminator[4] = {0x0D, 0x0A, 0x0D, 0x0A};

  mem=(char *)ck_alloc(mem_size);

  unsigned int cur_start = 0;
  unsigned int cur_end = 0;
  while (byte_count < buf_size) {

    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the last four bytes are 0x0D0A0D0A
    if ((mem_count >=4) && (memcmp(&mem[mem_count - 3], terminator, 4) == 0)) {
      region_count++;
      regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
      regions[region_count - 1].start_byte = cur_start;
      regions[region_count - 1].end_byte = cur_end;
      regions[region_count - 1].state_sequence = NULL;
      regions[region_count - 1].state_count = 0;

      mem_count = 0;
      cur_start = cur_end + 1;
      cur_end = cur_start;
    } else {
      mem_count++;
      cur_end++;

      //Check if the last byte has been reached
      if (cur_end == buf_size - 1) {
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;
        break;
      }

      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);

  //in case region_count equals zero, it means that the structure of the buffer is broken
  //hence we create one region for the whole buffer
  if ((region_count == 0) && (buf_size > 0)) {
    regions = (region_t *)ck_realloc(regions, sizeof(region_t));
    regions[0].start_byte = 0;
    regions[0].end_byte = buf_size - 1;
    regions[0].state_sequence = NULL;
    regions[0].state_count = 0;

    region_count = 1;
  }

  *region_count_ref = region_count;
  return regions;
}

region_t* extract_requests_ipp(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int region_count = 0;
  region_t *regions = NULL;
  char terminator[4] = {0x0D, 0x0A, 0x0D, 0x0A};
  char ipp[1] = {0x03};

  mem=(char *)ck_alloc(mem_size);

  unsigned int cur_start = 0;
  unsigned int cur_end = 0;
  while (byte_count < buf_size) {

    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the last bytes match the HTTP terminator (if data is sent) OR end-of-attributes-tag IPP command (if no data is sent)
    if ((mem_count > 3) && 
    ((memcmp(&mem[mem_count - 3], terminator, 4) == 0) || (memcmp(&mem[mem_count], ipp, 1) == 0)) &&
    ((buf_size - byte_count >= 4) && (memcmp(buf + byte_count, "POST", 4) == 0))
    ) {
      region_count++;
      regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
      regions[region_count - 1].start_byte = cur_start;
      regions[region_count - 1].end_byte = cur_end;
      regions[region_count - 1].state_sequence = NULL;
      regions[region_count - 1].state_count = 0;

      mem_count = 0;
      cur_start = cur_end + 1;
      cur_end = cur_start;
    } else {
      mem_count++;
      cur_end++;

      //Check if the last byte has been reached
      if (cur_end == buf_size - 1) {
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = cur_end;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;
        break;
      }

      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);

  //in case region_count equals zero, it means that the structure of the buffer is broken
  //hence we create one region for the whole buffer
  if ((region_count == 0) && (buf_size > 0)) {
    regions = (region_t *)ck_realloc(regions, sizeof(region_t));
    regions[0].start_byte = 0;
    regions[0].end_byte = buf_size - 1;
    regions[0].state_sequence = NULL;
    regions[0].state_count = 0;

    region_count = 1;
  }

  *region_count_ref = region_count;
  return regions;
}

unsigned int* extract_response_codes_smtp(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int *state_sequence = NULL;
  unsigned int state_count = 0;
  char terminator[2] = {0x0D, 0x0A};

  mem=(char *)ck_alloc(mem_size);

  state_count++;
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  state_sequence[state_count - 1] = 0;

  while (byte_count < buf_size) {
    memcpy(&mem[mem_count], buf + byte_count++, 1);

    if ((mem_count > 0) && (memcmp(&mem[mem_count - 1], terminator, 2) == 0)) {
      //Extract the response code which is the first 3 bytes
      char temp[4];
      memcpy(temp, mem, 4);
      temp[3] = 0x0;
      unsigned int message_code = (unsigned int) atoi(temp);

      if (message_code == 0) break;

      state_count++;
      state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
      state_sequence[state_count - 1] = message_code;
      mem_count = 0;
    } else {
      mem_count++;
      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);
  *state_count_ref = state_count;
  return state_sequence;
}

unsigned int* extract_response_codes_ssh(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
   char mem[7];
   unsigned int byte_count = 0;
   unsigned int *state_sequence = NULL;
   unsigned int state_count = 0;

   //Initial state
   state_count++;
   state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
   if (state_sequence == NULL) PFATAL("Unable realloc a memory region to store state sequence");
   state_sequence[state_count - 1] = 0;

   while (byte_count < buf_size) {
      memcpy(mem, buf + byte_count, 6);
      byte_count += 6;

      /* If this is the identification message */
      if (strstr(mem, "SSH")) {
        //Read until \x0D\x0A
        char tmp = 0x00;
        while (tmp != 0x0A) {
          memcpy(&tmp, buf + byte_count, 1);
          byte_count += 1;
        }
        state_count++;
        state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
        if (state_sequence == NULL) PFATAL("Unable realloc a memory region to store state sequence");
        state_sequence[state_count - 1] = 256; //Identification
      } else {
        //Extract the message type and skip the payload and the MAC
        unsigned int* size_buf = (unsigned int*)&mem[0];
        unsigned int message_size = (unsigned int)ntohl(*size_buf);

        //Break if the response does not adhere to the known format(s)
        //Normally, it only happens in the last response
        if (message_size - 2 > buf_size - byte_count) break;

        unsigned char message_code = (unsigned char)mem[5];
        state_count++;
        state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
        if (state_sequence == NULL) PFATAL("Unable realloc a memory region to store state sequence");
        state_sequence[state_count - 1] = message_code;
        /* If this is a KEY exchange related message */
        if ((message_code >= 20) && (message_code <= 49)) {
          //Do nothing
        } else {
          message_size += 8;
        }
        byte_count += message_size - 2;
      }
   }
   *state_count_ref = state_count;
   return state_sequence;
}

unsigned int* extract_response_codes_tls(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned char content_type, message_type;
  unsigned int *state_sequence = NULL;
  unsigned int state_count = 0;

  mem=(char *)ck_alloc(mem_size);

  //Add initial state
  state_count++;
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  state_sequence[state_count - 1] = 0;

  while (byte_count < buf_size) {

    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the region buffer length is at least 6 bytes (5 bytes for record header size)
    //the 6th byte could be message type
    if (mem_count >= 6) {
      //1st byte: content type
      //2nd and 3rd byte: TLS version
      //Extract the message size stored in the 4th and 5th bytes
      content_type = mem[0];

      //Check if this is an application data record
      if (content_type != 0x17) {
        message_type = mem[5];
      } else {
        message_type = 0xFF;
      }

      u16* size_buf = (u16*)&mem[3];
      u16 message_size = (u16)ntohs(*size_buf);

      //and skip the payload
      unsigned int bytes_to_skip = message_size - 1;
      unsigned int temp_count = 0;
      while ((byte_count < buf_size) && (temp_count < bytes_to_skip)) {
        byte_count++;
        temp_count++;
      }

      if (byte_count < buf_size) {
          byte_count--;
      }

      //add a new response code
      unsigned int message_code = (content_type << 8) + message_type;
      state_count++;
      state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
      state_sequence[state_count - 1] = message_code;
      mem_count = 0;
    } else {
      mem_count++;

      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);

  *state_count_ref = state_count;
  return state_sequence;
}

unsigned int* extract_response_codes_dicom(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
  if (buf_size == 0) {
    *state_count_ref = 0;
    return NULL;
  }

  unsigned int *state_sequence = NULL;
  unsigned int state_count = 0;

  state_count++;
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  state_sequence[state_count - 1] = 0; // initial status code is 0

  state_count++;
  unsigned int message_code = buf[0]; // return PDU type as status code
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  state_sequence[state_count - 1] = message_code;

  *state_count_ref = state_count;
  return state_sequence;
};

unsigned int* extract_response_codes_dns(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
  char *mem;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int *state_sequence = NULL;
  unsigned int state_count = 0;

  mem=(char *)ck_alloc(mem_size);

  state_count++;
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  state_sequence[state_count - 1] = 0;

  for (unsigned int byte_count = 0; byte_count < buf_size; byte_count++) {
    memcpy(&mem[mem_count], buf + byte_count, 1);

    // The original query will be included with the response.
    if ((mem_count >= 12) && (*(mem+mem_count) == 0)) {
      // 4 bytes left of the query. Jump to the answer.
      byte_count += 5;
      mem_count += 5;

      // Save the 3rd & 4th bytes as the response code
      unsigned int message_code = (unsigned int) ((mem[2] << 8) + mem[3]);

      state_count++;
      state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
      state_sequence[state_count - 1] = message_code;
      mem_count = 0;
    } else {
      mem_count++;
      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);
  *state_count_ref = state_count;
  return state_sequence;
}

static unsigned char dtls12_version[2] = {0xFE, 0xFD};

// (D)TLS known and custom constants

// the known 1-byte (D)TLS content types
#define CCS_CONTENT_TYPE 0x14
#define ALERT_CONTENT_TYPE 0x15
#define HS_CONTENT_TYPE 0x16
#define APPLICATION_CONTENT_TYPE 0x17
#define HEARTBEAT_CONTENT_TYPE 0x18

// custom content types
#define UNKNOWN_CONTENT_TYPE 0xFF // the content type is unrecognized

// custom handshake types (for handshake content)
#define UNKNOWN_MESSAGE_TYPE 0xFF // when the message type cannot be determined because the message is likely encrypted
#define MALFORMED_MESSAGE_TYPE 0xFE // when message type cannot be determined because the message appears to be malformed

region_t *extract_requests_dtls12(unsigned char* buf, unsigned int buf_size, unsigned int* region_count_ref) {
  unsigned int byte_count = 0;
  unsigned int region_count = 0;
  region_t *regions = NULL;

  unsigned int cur_start = 0;

   while (byte_count < buf_size) {

     //Check if the first three bytes are <valid_content_type><dtls-1.2>
     if ((byte_count > 3 && buf_size - byte_count > 1) &&
     (buf[byte_count] >= CCS_CONTENT_TYPE && buf[byte_count] <= HEARTBEAT_CONTENT_TYPE)  &&
     (memcmp(&buf[byte_count+1], dtls12_version, 2) == 0)) {
       region_count++;
       regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
       regions[region_count - 1].start_byte = cur_start;
       regions[region_count - 1].end_byte = byte_count-1;
       regions[region_count - 1].state_sequence = NULL;
       regions[region_count - 1].state_count = 0;
       cur_start = byte_count;
     } else {

      //Check if the last byte has been reached
      if (byte_count == buf_size - 1) {
        region_count++;
        regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));
        regions[region_count - 1].start_byte = cur_start;
        regions[region_count - 1].end_byte = byte_count;
        regions[region_count - 1].state_sequence = NULL;
        regions[region_count - 1].state_count = 0;
        break;
      }
     }

     byte_count ++;
  }

  //in case region_count equals zero, it means that the structure of the buffer is broken
  //hence we create one region for the whole buffer
  if ((region_count == 0) && (buf_size > 0)) {
    regions = (region_t *)ck_realloc(regions, sizeof(region_t));
    regions[0].start_byte = 0;
    regions[0].end_byte = buf_size - 1;
    regions[0].state_sequence = NULL;
    regions[0].state_count = 0;

    region_count = 1;
  }

  *region_count_ref = region_count;
  return regions;
}

// a status code comprises <content_type, message_type> tuples
// message_type varies depending on content_type (e.g. for handshake content, message_type is the handshake message type...)
//
unsigned int* extract_response_codes_dtls12(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
  unsigned int byte_count = 0;
  unsigned int *state_sequence = NULL;
  unsigned int state_count = 0;
  unsigned int status_code = 0;

  state_count++;
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  state_sequence[state_count - 1] = 0; // initial status code is 0

  while (byte_count < buf_size) {
    // a DTLS 1.2 record has a 13 bytes header, followed by the contained message
    if ( (buf_size - byte_count > 13) &&
    (buf[byte_count] >= CCS_CONTENT_TYPE && buf[byte_count] <= HEARTBEAT_CONTENT_TYPE)  &&
    (memcmp(&buf[byte_count+1], dtls12_version, 2) == 0)) {
      unsigned char content_type = buf[byte_count];
      unsigned char message_type;
      u32 record_length = read_bytes_to_uint32(buf, byte_count+11, 2);

      // the record length exceeds buffer boundaries (not expected)
      if (buf_size - byte_count - 13 - record_length < 0) {
        message_type = MALFORMED_MESSAGE_TYPE;
      }
      else {
        switch(content_type) {
          case HS_CONTENT_TYPE: ;
            unsigned char hs_msg_type = buf[byte_count+13];
            // the minimum size of a correct DTLS 1.2 handshake message is 12 bytes comprising fragment header fields
            if (record_length >= 12) {
              u32 frag_length = read_bytes_to_uint32(buf, byte_count+22, 3);
              // we can check if the handshake record is encrypted by subtracting fragment length from record length
              // which should yield 12 if the fragment is not encrypted
              // the likelyhood for an encrypted fragment to satisfy this condition is very small
              if (record_length - frag_length == 12) {
                // not encrypted
                message_type = hs_msg_type;
              } else {
                // encrypted handshake message
                message_type = UNKNOWN_MESSAGE_TYPE;
              }
            } else {
                // malformed handshake message
                message_type = MALFORMED_MESSAGE_TYPE;
            }
          break;
          case CCS_CONTENT_TYPE:
            if (record_length == 1) {
              // unencrypted CCS
              unsigned char ccs_msg_type = buf[byte_count+13];
              message_type = ccs_msg_type;
            } else {
              if (record_length > 1) {
                // encrypted CCS
                message_type = UNKNOWN_MESSAGE_TYPE;
              } else {
                // malformed CCS
                message_type = MALFORMED_MESSAGE_TYPE;
              }
            }
          break;
          case ALERT_CONTENT_TYPE:
            if (record_length == 2) {
              // unencrypted alert, the type is sufficient for determining which alert occurred
              // unsigned char level = buf[byte_count+13];
              unsigned char type = buf[byte_count+14];
              message_type = type;
            } else {
              if (record_length > 2) {
                // encrypted alert
                message_type = UNKNOWN_MESSAGE_TYPE;
              } else {
                // malformed alert
                message_type = MALFORMED_MESSAGE_TYPE;
              }
            }
          break;
          case APPLICATION_CONTENT_TYPE:
            // for application messages we cannot determine whether they are encrypted or not
            message_type = UNKNOWN_MESSAGE_TYPE;
          break;
          case HEARTBEAT_CONTENT_TYPE:
            // a heartbeat message is at least 3 bytes long (1 byte type, 2 bytes payload length)
            // unfortunately, telling an encrypted message from an unencrypted message cannot be done reliably due to the variable length of padding
            // hence we just use unknown for either case
            if (record_length >= 3) {
              // unsigned char hb_msg_type = buf[byte_count+13];
              // u32 hb_length = read_bytes_to_uint32(buf, byte_count+14, 2);
              // unkown heartbeat message
              message_type = UNKNOWN_MESSAGE_TYPE;
            } else {
              // malformed heartbeat
              message_type = MALFORMED_MESSAGE_TYPE;
            }
          break;
          default:
            // unknown content and message type, should not be hit
            content_type = UNKNOWN_CONTENT_TYPE;
            message_type = UNKNOWN_MESSAGE_TYPE;
          break;
        }
      }

      status_code = (content_type << 8) + message_type;
      state_count++;
      state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
      state_sequence[state_count - 1] = status_code;
      byte_count += record_length;
    } else {
      // we shouldn't really be reaching this code
      byte_count ++;
    }
  }

  *state_count_ref = state_count;
  return state_sequence;
}

unsigned int* extract_response_codes_rtsp(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int *state_sequence = NULL;
  unsigned int state_count = 0;
  char terminator[2] = {0x0D, 0x0A};
  char rtsp[5] = {0x52, 0x54, 0x53, 0x50, 0x2f};

  mem=(char *)ck_alloc(mem_size);

  state_count++;
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  state_sequence[state_count - 1] = 0;

  while (byte_count < buf_size) {
    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the last two bytes are 0x0D0A
    if ((mem_count > 0) && (memcmp(&mem[mem_count - 1], terminator, 2) == 0)) {
      if ((mem_count >= 5) && (memcmp(mem, rtsp, 5) == 0)) {
        //Extract the response code which is the first 3 bytes
        char temp[4];
        memcpy(temp, &mem[9], 4);
        temp[3] = 0x0;
        unsigned int message_code = (unsigned int) atoi(temp);

        if (message_code == 0) break;

        state_count++;
        state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
        state_sequence[state_count - 1] = message_code;
        mem_count = 0;
      } else {
        mem_count = 0;
      }
    } else {
      mem_count++;
      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);
  *state_count_ref = state_count;
  return state_sequence;
}

unsigned int* extract_response_codes_ftp(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int *state_sequence = NULL;
  unsigned int state_count = 0;
  char terminator[2] = {0x0D, 0x0A};

  mem=(char *)ck_alloc(mem_size);

  state_count++;
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  state_sequence[state_count - 1] = 0;

  while (byte_count < buf_size) {
    memcpy(&mem[mem_count], buf + byte_count++, 1);

    if ((mem_count > 0) && (memcmp(&mem[mem_count - 1], terminator, 2) == 0)) {
      //Extract the response code which is the first 3 bytes
      char temp[4];
      memcpy(temp, mem, 4);
      temp[3] = 0x0;
      unsigned int message_code = (unsigned int) atoi(temp);

      if (message_code == 0) break;

      state_count++;
      state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
      state_sequence[state_count - 1] = message_code;
      mem_count = 0;
    } else {
      mem_count++;
      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);
  *state_count_ref = state_count;
  return state_sequence;
}

unsigned int* extract_response_codes_sip(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int *state_sequence = NULL;
  unsigned int state_count = 0;
  char terminator[2] = {0x0D, 0x0A};
  char sip[4] = {0x53, 0x49, 0x50, 0x2f};

  mem=(char *)ck_alloc(mem_size);

  state_count++;
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  state_sequence[state_count - 1] = 0;

  while (byte_count < buf_size) {
    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the last two bytes are 0x0D0A
    if ((mem_count > 0) && (memcmp(&mem[mem_count - 1], terminator, 2) == 0)) {
      if ((mem_count >= 4) && (memcmp(mem, sip, 4) == 0)) {
        //Extract the response code which is the first 3 bytes
        char temp[4];
        memcpy(temp, &mem[8], 4);
        temp[3] = 0x0;
        unsigned int message_code = (unsigned int) atoi(temp);

        if (message_code == 0) break;

        state_count++;
        state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
        state_sequence[state_count - 1] = message_code;
        mem_count = 0;
      } else {
        mem_count = 0;
      }
    } else {
      mem_count++;
      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);
  *state_count_ref = state_count;
  return state_sequence;
}

unsigned int* extract_response_codes_http(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int *state_sequence = NULL;
  unsigned int state_count = 0;
  char terminator[2] = {0x0D, 0x0A};
  char http[5] = {0x48, 0x54, 0x54, 0x50, 0x2f};

  mem=(char *)ck_alloc(mem_size);

  state_count++;
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  state_sequence[state_count - 1] = 0;

  while (byte_count < buf_size) {
    memcpy(&mem[mem_count], buf + byte_count++, 1);

    //Check if the last two bytes are 0x0D0A
    if ((mem_count > 0) && (memcmp(&mem[mem_count - 1], terminator, 2) == 0)) {
      if ((mem_count >= 5) && (memcmp(mem, http, 5) == 0)) {
        //Extract the response code which is the first 3 bytes
        char temp[4];
        memcpy(temp, &mem[9], 4);
        temp[3] = 0x0;
        unsigned int message_code = (unsigned int) atoi(temp);

        if (message_code == 0) break;

        state_count++;
        state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
        state_sequence[state_count - 1] = message_code;
        mem_count = 0;
      } else {
        mem_count = 0;
      }
    } else {
      mem_count++;
      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }
  if (mem) ck_free(mem);
  *state_count_ref = state_count;
  return state_sequence;
}

unsigned int* extract_response_codes_ipp(unsigned char* buf, unsigned int buf_size, unsigned int* state_count_ref)
{
  char *mem;
  unsigned int byte_count = 0;
  unsigned int mem_count = 0;
  unsigned int mem_size = 1024;
  unsigned int *state_sequence = NULL;
  unsigned int state_count = 0;
  char terminatorHTTP[4] = {0x0D, 0x0A, 0x0D, 0x0A};
  char http[5] = {0x48, 0x54, 0x54, 0x50, 0x2F};
  unsigned int message_code = 0;
  char tempHTTP[4];

  mem = (char *)ck_alloc(mem_size);

  //Initial state
  state_count++;
  state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));
  if (state_sequence == NULL) PFATAL("Unable realloc a memory region to store state sequence");
  state_sequence[state_count - 1] = 0;

  while (byte_count < buf_size) {
    memcpy(&mem[mem_count], buf + byte_count++, 1);
    //Check if the last two bytes are 0x0D0A0D0A
    if ((mem_count > 3) && (memcmp(&mem[mem_count - 3], terminatorHTTP, 4) == 0)) {
      if ((mem_count >= 5) && (memcmp(mem, http, 5) == 0)) {
        
        memcpy(tempHTTP, &mem[9], 4);
        tempHTTP[3] = 0x0;
        message_code = (unsigned int) atoi(tempHTTP);

        if (message_code == 0) break;
        
        if (message_code == 200) {
          //Extract IPP response code (bytes 3 and 4)
          unsigned int third = (unsigned int) buf[byte_count + 2];
          unsigned int fourth = (unsigned int) buf[byte_count + 3];

          //200 + IPP code to not confuse initial 0 state with successful-ok 0 state
          message_code += (unsigned int) (10 * third + fourth);
        }

        state_count++;
        state_sequence = (unsigned int *)ck_realloc(state_sequence, state_count * sizeof(unsigned int));

        if (state_sequence == NULL) PFATAL("Unable realloc a memory region to store state sequence");
        
        state_sequence[state_count - 1] = message_code;       

        mem_count = 0;
      } else {
        mem_count = 0;
      }

    } else {

      mem_count++;
      if (mem_count == mem_size) {
        //enlarge the mem buffer
        mem_size = mem_size * 2;
        mem=(char *)ck_realloc(mem, mem_size);
      }
    }
  }

  if (mem) ck_free(mem);
  *state_count_ref = state_count;
  return state_sequence;
}

// kl_messages manipulating functions

klist_t(lms) *construct_kl_messages(u8* fname, region_t *regions, u32 region_count)
{
  FILE *fseed = NULL;
  fseed = fopen(fname, "rb");
  if (fseed == NULL) PFATAL("Cannot open seed file %s", fname);

  klist_t(lms) *kl_messages = kl_init(lms);
  u32 i;

  for (i = 0; i < region_count; i++) {
    //Identify region size
    u32 len = regions[i].end_byte - regions[i].start_byte + 1;

    //Create a new message
    message_t *m = (message_t *) ck_alloc(sizeof(message_t));
    m->mdata = (char *) ck_alloc(len);
    m->msize = len;
    if (m->mdata == NULL) PFATAL("Unable to allocate memory region to store new message");
    fread(m->mdata, 1, len, fseed);

    //Insert the message to the linked list
    *kl_pushp(lms, kl_messages) = m;
  }

  if (fseed != NULL) fclose(fseed);
  return kl_messages;
}

void delete_kl_messages(klist_t(lms) *kl_messages)
{
  /* Free all messages in the list before destroying the list itself */
  message_t *m;

  int ret = kl_shift(lms, kl_messages, &m);
  while (ret == 0) {
    if (m) {
      ck_free(m->mdata);
      ck_free(m);
    }
    ret = kl_shift(lms, kl_messages, &m);
  }

  /* Finally, destroy the list */
	kl_destroy(lms, kl_messages);
}

kliter_t(lms) *get_last_message(klist_t(lms) *kl_messages)
{
  kliter_t(lms) *it;
  it = kl_begin(kl_messages);
  while (kl_next(it) != kl_end(kl_messages)) {
    it = kl_next(it);
  }
  return it;
}


u32 save_kl_messages_to_file(klist_t(lms) *kl_messages, u8 *fname, u8 replay_enabled, u32 max_count)
{
  u8 *mem = NULL;
  u32 len = 0, message_size = 0;
  kliter_t(lms) *it;

  s32 fd = open(fname, O_WRONLY | O_CREAT, 0600);
  if (fd < 0) PFATAL("Unable to create file '%s'", fname);

  u32 message_count = 0;
  //Iterate through all messages in the linked list
  for (it = kl_begin(kl_messages); it != kl_end(kl_messages) && message_count < max_count; it = kl_next(it)) {
    message_size = kl_val(it)->msize;
    if (replay_enabled) {
		  mem = (u8 *)ck_realloc(mem, 4 + len + message_size);

      //Save packet size first
      u32 *psize = (u32*)&mem[len];
      *psize = message_size;

      //Save packet content
      memcpy(&mem[len + 4], kl_val(it)->mdata, message_size);
      len = 4 + len + message_size;
    } else {
      mem = (u8 *)ck_realloc(mem, len + message_size);

      //Save packet content
      memcpy(&mem[len], kl_val(it)->mdata, message_size);
      len = len + message_size;
    }
    message_count++;
  }

  //Write everything to file & close the file
  ck_write(fd, mem, len, fname);
  close(fd);

  //Free the temporary buffer
  ck_free(mem);

  return len;
}

region_t* convert_kl_messages_to_regions(klist_t(lms) *kl_messages, u32* region_count_ref, u32 max_count)
{
  region_t *regions = NULL;
  kliter_t(lms) *it;

  u32 region_count = 1;
  s32 cur_start = 0, cur_end = 0;
  //Iterate through all messages in the linked list
  for (it = kl_begin(kl_messages); it != kl_end(kl_messages) && region_count <= max_count ; it = kl_next(it)) {
    regions = (region_t *)ck_realloc(regions, region_count * sizeof(region_t));

    cur_end = cur_start + kl_val(it)->msize - 1;
    if (cur_end < 0) PFATAL("End_byte cannot be negative");

    regions[region_count - 1].start_byte = cur_start;
    regions[region_count - 1].end_byte = cur_end;
    regions[region_count - 1].state_sequence = NULL;
    regions[region_count - 1].state_count = 0;

    cur_start = cur_end + 1;
    region_count++;
  }

  *region_count_ref = region_count - 1;
  return regions;
}

// Network communication functions

int net_send(int sockfd, struct timeval timeout, char *mem, unsigned int len) {
  unsigned int byte_count = 0;
  int n;
  struct pollfd pfd[1];
  pfd[0].fd = sockfd;
  pfd[0].events = POLLOUT;
  int rv = poll(pfd, 1, 1);

  setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
  if (rv > 0) {
    if (pfd[0].revents & POLLOUT) {
      while (byte_count < len) {
        usleep(10);
        n = send(sockfd, &mem[byte_count], len - byte_count, MSG_NOSIGNAL);
        if (n == 0) return byte_count;
        if (n == -1) return -1;
        byte_count += n;
      }
    }
  }
  return byte_count;
}

int net_recv(int sockfd, struct timeval timeout, int poll_w, char **response_buf, unsigned int *len) {
  char temp_buf[1000];
  int n;
  struct pollfd pfd[1];
  pfd[0].fd = sockfd;
  pfd[0].events = POLLIN;
  int rv = poll(pfd, 1, poll_w);

  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
  // data received
  if (rv > 0) {
    if (pfd[0].revents & POLLIN) {
      n = recv(sockfd, temp_buf, sizeof(temp_buf), 0);
      if ((n < 0) && (errno != EAGAIN)) {
        return 1;
      }
      while (n > 0) {
        usleep(10);
        *response_buf = (unsigned char *)ck_realloc(*response_buf, *len + n + 1);
        memcpy(&(*response_buf)[*len], temp_buf, n);
        (*response_buf)[(*len) + n] = '\0';
        *len = *len + n;
        n = recv(sockfd, temp_buf, sizeof(temp_buf), 0);
        if ((n < 0) && (errno != EAGAIN)) {
          return 1;
        }
      }
    }
  } else
    if (rv < 0) // an error was returned
      return 1;

  // rv == 0 poll timeout or all data pending after poll has been received successfully
  return 0;
}

// Utility function

void save_regions_to_file(region_t *regions, unsigned int region_count, unsigned char *fname)
{
  int fd;
  FILE* fp;

  fd = open(fname, O_WRONLY | O_CREAT | O_EXCL, 0600);

  if (fd < 0) return;

  fp = fdopen(fd, "w");

  if (!fp) {
    close(fd);
    return;
  }

  int i;

  for(i=0; i < region_count; i++) {
     fprintf(fp, "Region %d - Start: %d, End: %d\n", i, regions[i].start_byte, regions[i].end_byte);
  }

  fclose(fp);
}

int str_split(char* a_str, const char* a_delim, char **result, int a_count)
{
	char *token;
	int count = 0;

	/* count number of tokens */
	/* get the first token */
	char* tmp1 = strdup(a_str);
	token = strtok(tmp1, a_delim);

	/* walk through other tokens */
	while (token != NULL)
	{
		count++;
		token = strtok(NULL, a_delim);
	}

	if (count != a_count)
	{
		return 1;
	}

	/* split input string, store tokens into result */
	count = 0;
	/* get the first token */
	token = strtok(a_str, a_delim);

	/* walk through other tokens */

	while (token != NULL)
	{
		result[count] = token;
		count++;
		token = strtok(NULL, a_delim);
	}

	free(tmp1);
	return 0;
}

void str_rtrim(char* a_str)
{
	char* ptr = a_str;
	int count = 0;
	while ((*ptr != '\n') && (*ptr != '\t') && (*ptr != ' ') && (count < strlen(a_str))) {
		ptr++;
		count++;
	}
	if (count < strlen(a_str)) {
		*ptr = '\0';
	}
}

int parse_net_config(u8* net_config, u8* protocol, u8** ip_address, u32* port)
{
  char  buf[80];
  char **tokens;
  int tokenCount = 3;

  tokens = (char**)malloc(sizeof(char*) * (tokenCount));

  if (strlen(net_config) > 80) return 1;

  strncpy(buf, net_config, strlen(net_config));
   str_rtrim(buf);

  if (!str_split(buf, "/", tokens, tokenCount))
  {
      if (!strcmp(tokens[0], "tcp:")) {
        *protocol = PRO_TCP;
      } else if (!strcmp(tokens[0], "udp:")) {
        *protocol = PRO_UDP;
      } else return 1;

      //TODO: check the format of this IP address
      *ip_address = strdup(tokens[1]);

      *port = atoi(tokens[2]);
      if (*port == 0) return 1;
  } else return 1;
  return 0;
}

u8* state_sequence_to_string(unsigned int *stateSequence, unsigned int stateCount) {
  u32 i = 0;

  u8 *out = NULL;

  char strState[STATE_STR_LEN];
  size_t len = 0;
  for (i = 0; i < stateCount; i++) {
    //Limit the loop to shorten the output string
    if ((i >= 2) && (stateSequence[i] == stateSequence[i - 1]) && (stateSequence[i] == stateSequence[i - 2])) continue;
    unsigned int stateID = stateSequence[i];
    if (i == stateCount - 1) {
      snprintf(strState, STATE_STR_LEN, "%d", (int) stateID);
    } else {
      snprintf(strState, STATE_STR_LEN, "%d-", (int) stateID);
    }
    out = (u8 *)ck_realloc(out, len + strlen(strState) + 1);
    memcpy(&out[len], strState, strlen(strState) + 1);
    len=strlen(out);
    //As Linux limit the size of the file name
    //we set a fixed upper bound here
    if (len > 150 && (i + 1 < stateCount)) {
      snprintf(strState, STATE_STR_LEN, "%s", "end-at-");
      out = (u8 *)ck_realloc(out, len + strlen(strState) + 1);
      memcpy(&out[len], strState, strlen(strState) + 1);
      len=strlen(out);

      snprintf(strState, STATE_STR_LEN, "%d", (int) stateSequence[stateCount - 1]);
      out = (u8 *)ck_realloc(out, len + strlen(strState) + 1);
      memcpy(&out[len], strState, strlen(strState) + 1);
      len=strlen(out);
      break;
    }
  }
  return out;
}


void hexdump(unsigned char *msg, unsigned char * buf, int start, int end) {
  printf("%s : ", msg);
  for (int i=start; i<=end; i++) {
    printf("%02x", buf[i]);
  }
  printf("\n");
}


u32 read_bytes_to_uint32(unsigned char* buf, unsigned int offset, int num_bytes) {
  u32 val = 0;
  for (int i=0; i<num_bytes; i++) {
    val = (val << 8) + buf[i+offset];
  }
  return val;
}
