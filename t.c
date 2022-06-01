#include <stdio.h>
#include <string.h>

void f(int *map_a, int *map_b, int *min_counter, int *exact_counter, int *counter_finalized, int *avoid_pos, int *exact_pos, int *chars_avoid, char *a, char *b, char *format)
{
  printf("%s VS %s\n", a, b);

  int min_counter_b[256] = {0};

  memset((void*)format, 0, strlen(format));
  memset((void*)map_a, 0, sizeof(int)*256);
  memset((void*)map_b, 0, sizeof(int)*256);

  for (int i = 0; i < strlen(a); i++)
  {
    map_a[a[i]]++;
    map_b[b[i]]++;
  }

  for (int i = 0; i < 256; i++)
  {
    if (map_a[i]) printf("c : %c %d\n", i, map_a[i]);
  }

  int copy_a_map[256] = {0};
  memcpy(copy_a_map, map_a, sizeof(int)*256);

  for (int i = 0; i < strlen(a); i++)
  {
    if (a[i] == b[i])
    {
      format[i] = '+';
      /* bring the counter to min values */
      min_counter_b[a[i]]++;

      /* sign the exact positions */
      /* the char a[i] must be at index i */
      exact_pos[a[i]] = i;
      printf("do have %c at %d\n", a[i], exact_pos[a[i]]);
      
      /* update positional map */
      map_a[a[i]]--;
      map_b[a[i]]--;

      if (a[i] == '-')
      {
        printf("remained+: %d\n", map_a[a[i]]);
      } 
    }
  }

  for (int i = 0; i < strlen(a); i++)
  {
    if (format[i]) continue;

    if (map_b[b[i]] && !map_a[b[i]])
    {
      if (b[i] == '-') printf("A\n");
      map_b[b[i]]--;
      format[i] = '/';

      /* finalize the counter */
      counter_finalized[b[i]] = 1;
      exact_counter[b[i]] = min_counter[b[i]];
      
      /* sign the chars to avoid if not present in target map  */
      if (copy_a_map[b[i]] == 0)
      {
        chars_avoid[b[i]] = 1;
      }

      /* maintain the exact map as we know we have reached the max num for this char */
    }
    else if (map_b[b[i]] && map_a[b[i]])
    {
      printf("remained: %d\n", map_a[b[i]]);
      if (b[i] == '-') printf("B\n");
      map_b[b[i]]--;
      map_a[b[i]]--;
      format[i] = '|';

      /* sign position to avoid */
      /* avoid b[i] at index i */
      avoid_pos[b[i]] = i;

      /* increment the min counter */
      min_counter_b[b[i]]++;
    }
  }
  format[strlen(a)] = '\0';

  for (int i = 0; i < 256; i++)
  {
    min_counter[i] = (min_counter[i] > min_counter_b[i] ? min_counter[i] : min_counter_b[i]);
  }

  printf("format: %s\n", format);

  for (int i = 0; i < 256; i++)
  {
    if (min_counter[i])
    {
      printf("min %c: %d\n", i, min_counter[i]);
    }
    if (counter_finalized[i])
    {
      printf("SURE %c : %d\n", i, exact_counter[i]);
    }
  }

  for (int i = 0; i < 256; i++)
  {
    //if (exact_pos[i] >= 0)
    //{
    //  printf("do have %c at %d\n", i, exact_pos[i]);
    //}
    if (avoid_pos[i] >= 0)
    {
      printf("do NOT have %c at %d\n", i, avoid_pos[i]);
    }
    if (chars_avoid[i])
    {
      printf("avoid total: %c\n", i);  
    }
  }
}

int main()
{
  char a[] = "djPDi939-s__e-s";
  char b[] = "gioSON-we2_w234";
  char c[] = "-ioSON-we2_we-4";
  char d[] = "aaaaaaaaa55_665";

  char format[sizeof(a)] = {0};

  int map_a[256] = {0};
  int map_b[256] = {0};

  int min_counter[256] = {0};
  int exact_counter[256] = {0};

  int counter_finalized[256] = {0};

  int exact_pos[256] = {0};
  int avoid_pos[256] = {0};

  int chars_avoid[256] = {0};

  memset((void*)exact_pos, -1, sizeof(int)*256);
  memset((void*)avoid_pos, -1, sizeof(int)*256);

  f(map_a, map_b, min_counter, exact_counter, counter_finalized, avoid_pos, exact_pos, chars_avoid, a, b, format);
  f(map_a, map_b, min_counter, exact_counter, counter_finalized, avoid_pos, exact_pos, chars_avoid, a, c, format);
  f(map_a, map_b, min_counter, exact_counter, counter_finalized, avoid_pos, exact_pos, chars_avoid, a, d, format);
}
