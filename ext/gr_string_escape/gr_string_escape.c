#include "ruby.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
   

#define MAX_STACK_DEPTH 20
#define MAX_URL_PRINT_SIZE 40
#define MAX_ATTRIBUTES 10
#define DYNAMICS_INCREMENT 128
#define STRONG "strong"
#define EM "em"
#define FALSE 0
#define TRUE 1
#define EMPTY_STRING ""

static int id_push; // ruby thing

int input_size, output_size, max_output_size, absolute_url_size;
int position;
int ahead_position;
char **tag_stack;
int tag_stack_size;
int anchors_in_stack;
int cap_count;
int writing_utf_8;  //set to the size of the utf_8 char remaining
int counting_cap;

char* input;
char* absolute_url;
char* output;

static void downcase(char *string) {
  int i;
  for(i=0; string[i]; i++) {
    if (string[i] >= 'A' && string[i] <= 'Z'){
      string[i] += 32;
    }
  }
}

static void downcasen(char *string, int string_size) {
  int i;
  //downcase
  for(i=0; i < string_size; i++) {
    if (string[i] >= 'A' && string[i] <= 'Z'){
      string[i] += 32;
    }
  }
}

static void write_chars(char *chars) {
  int size = strlen(chars);
  if(output_size + size >= max_output_size) {
    printf("Error: max_output_size is being exceeded\n");
    return;
  }
  strncpy(output + output_size, chars, size);
  output_size += size;
  if(counting_cap) {
    cap_count += size;
  }
}

inline push_to_tag_stack(char *string, int string_size) {
  char *new_string;
  new_string = ALLOC_N(char, string_size + 1);
  strcpy(new_string, string);
  tag_stack[tag_stack_size++] = new_string;
}

inline write_nchars(char *chars, int chars_size) {
  if(output_size + chars_size >= max_output_size) {
    printf("Error: max_output_size is being exceeded\n");
    return;
  }
  strncpy(output + output_size, chars, chars_size);
  output_size += chars_size;
  if(counting_cap) {
    cap_count += chars_size;
  }
}

inline void write_char(char char_to_write) {
  if(output_size + 1 >= max_output_size) {
    printf("Error: max_output_size is being exceeded\n");
    return;
  }
  
  output[output_size++] = char_to_write;
  if (writing_utf_8) {
    if(char_to_write & 0x80 && !(char_to_write & 0x40)) {
      writing_utf_8 --;
    }
    else {
      writing_utf_8 = 0;
    }
  }
  else {
    if(char_to_write & 0x80) {
      if (char_to_write & 0x40) {
        writing_utf_8 ++;
        if (char_to_write & 0x20) {
          writing_utf_8 ++;
          if (char_to_write & 0x10) {
            writing_utf_8 ++;
          }
        }
      }
    }
  }
  if(counting_cap && !writing_utf_8) {
    cap_count++;
  }
}


static void write_escaped_chars(char *chars) {
  int i = 0;
  char c;
  for (i = 0; c = chars[i]; i++) {
    switch(c) {
    case '&':
      write_chars("&amp;");
      break;
    case '>':
      write_chars("&gt;");
      break;
    case '<':
      write_chars("&lt;");
      break;
    case '"':
      write_chars("&quot;");
      break;
    default:
      write_char(c);
      break;
    }
  }
}




static void write_urlitized_chars(char *chars) {
  int i = 0;
  int wrote_underscore = FALSE;
  char c;
  for (i = 0; c = chars[i]; i++) {
    if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
       (c >= '0' && c <= '9')) {
      write_char(c);
      wrote_underscore = FALSE;
    }
    else {
      if (!wrote_underscore) {
        wrote_underscore = TRUE;
        write_char('_');
      }
    }
  }
}

static int valid_url(char * string){
  if (!string || strlen(string) <= 4) {
    return FALSE;
  }
  // disallow javascript tags
  if(string[0] != 'j' && string[0] != 'J') {
    return TRUE;
  }
  
  if(string[1] != 'a' && string[1] != 'A') {
    return TRUE;
  }
  
  if(string[2] != 'v' && string[2] != 'V') {
    return TRUE;
  }
  
  if(string[3] != 'a' && string[3] != 'A') {
    return TRUE;
  }
  return FALSE;
}


static int attribute_find(char *key, char **keys, int num_keys) {
  int i;
  
  // printf("finding attr: %s\n", key);
  for(i=0; i< num_keys; i++) {
    if(strcmp(key, keys[i]) == 0) {
      // printf("found!: %d\n", i);
      return i;
    }
  }
  return -1;
}

static int html_parse(int start, int finish) {
  int close_tag = FALSE;
  int tag_closed = FALSE;
  char *tag;
  char *name;
  char c;
  int i;
  char *attribute_keys[MAX_ATTRIBUTES];
  char *attribute_values[MAX_ATTRIBUTES];
  int num_attributes = 0;
  int tag_size;
  int name_size = 0;
  
  tag = input + start;
  tag_size = finish - start;
  
  // printf("tag start: %c tag_size:%d\n", tag[0], tag_size);
  if(tag_size <= 0) {
    return FALSE;
  }
  
  // read all the whitespace and first slash "< / "
  for(; tag_size > 0; tag++, tag_size--) {
    int done = FALSE;
    switch(c = tag[0]) {
    case '/':
      if (close_tag){
        return FALSE;
      }
      else {
        close_tag = TRUE;
      }
      break;
    case ' ':
    case '\t':
    case '\n':
      // ignore
      break;
    default:
      done = TRUE;
      break;
    }
    if(done){
      break;
    }
  }
  
  // read all the whitespace and last slash
  for(; tag_size > 0; tag_size--) {
    int done = FALSE;
    switch(c = tag[tag_size - 1]) {
    case '/':
      if (tag_closed){
        return FALSE;
      }
      else {
        tag_closed = TRUE;
      }
      break;
    case ' ':
    case '\t':
    case '\n':
      // ignore
      break;
    default:
      done = TRUE;
      break;
    }
    if(done){
      break;
    }
  }
  
  if(tag_size == 0) {
    return FALSE;
  }
  
  //read name
  for(name_size = 0; name_size < tag_size; name_size++) {
    int done = FALSE;
    switch(tag[name_size]) {
    case ' ':
    case '\t':
    case '\n':
      done = TRUE;
      break;
    }
    if(done) {
      break;
    }
  }
  name = ALLOCA_N(char, name_size + 1);
  strncpy(name, tag, name_size);
  name[name_size] = 0;
  
  // printf("name_size %d  tag name: %s\n", name_size, name);
  
  tag_size -= name_size;
  tag += name_size;
  
  
  // read attributes
  for(; tag_size > 0; tag_size--, tag++) {
    char *key = tag;
    char *value = NULL;
    int key_size = 0;
    int value_size = 0; // would you like fries with that?
    int skip_value = FALSE;
     
    // read key
    for(; key_size < tag_size; key_size++) {  
      int done = FALSE;
      c = tag[key_size];
      switch(c) {
      case ' ':
      case '\t':
      case '\n':
        if (key_size > 0) {
          done = TRUE;
        }
        else {
          //ignore preceding whitespace
          key++;
          key_size--;
          tag++;
          tag_size--;
        }
        break;
      case '=':
        if (key_size > 0) {
          done = TRUE;
          // do not advance, equals still needs to be found
        }
        else {
          return FALSE;
        }
        break;
      case '\'':
      case '"':
        return FALSE;
      }
      if(done) {
        break;
      }
    } // got key
    key = ALLOCA_N(char, key_size + 1);
    strncpy(key, tag, key_size);
    key[key_size] = 0;
    tag += key_size;
    tag_size -= key_size;
    
    for(;tag_size > 0; tag++, tag_size--) {
      int done = FALSE;
      switch(tag[0]) {
      case ' ':
      case '\t':
      case '\n':
        //ignore whitespace
        break;
      case '=':
        done = TRUE;
        tag++;
        tag_size--;
        break;
      default:
        skip_value = TRUE;
        done = TRUE;
        break;
      }
      if(done) {
        break;
      }
    }
  
    if(!skip_value) {
      // read value
      int started_reading_value = FALSE;
      for(; value_size < tag_size; value_size++) {
        int done = FALSE;
        c = tag[value_size];
        switch(c) {
        case ' ':
        case '\t':
        case '\n':
          if (started_reading_value) {
            done = TRUE;
          }
          else {
            //ignore preceding whitespace
            value_size--;
            tag++;
            tag_size--;
          }
          break;
        case '=':
          return FALSE;
        case '\'':
        case '"':
          if(!value) {
            started_reading_value = TRUE;
            tag++;
            tag_size --;
            while(value_size <= tag_size && tag[value_size] != c) {
              value_size++;
            }
            if(tag[value_size] != c){
              return FALSE;
            }
            done = TRUE;
          }
          else {
            return FALSE;
          }
          break;
        default:
          if (!started_reading_value) {
            started_reading_value = TRUE;
          }
          break;
        }
        if(done) {
          break;
        }
      }
      
      if(started_reading_value) {
        value = ALLOCA_N(char, value_size + 1);
        strncpy(value, tag, value_size);
        value[value_size] = 0;
        tag += value_size;
        tag_size -= value_size;
      }
    }
    
    if (key_size > 0) {
      if (num_attributes >= MAX_ATTRIBUTES) {
        break;
      }
      attribute_keys[num_attributes] = key;
      if (value) {
        attribute_values[num_attributes] = value;        
      }
      else {
        attribute_values[num_attributes] = EMPTY_STRING;
      }
      num_attributes ++;
    }
  }
  
  for(i = 0; i< num_attributes; i++) {
    downcase(attribute_keys[i]);
  }
  
  //clean tag!
  downcasen(name, name_size);
  if(strcmp(name, "b") == 0){
    name = STRONG;
  }
  else if(strcmp(name, "i") == 0) {
    name = EM;
  }
  
  if(close_tag) {
    if(tag_stack_size == 0){
      return FALSE;
    }
    if(strcmp(tag_stack[tag_stack_size - 1], name) == 0){
      if(strcmp(name, "a")) {
        anchors_in_stack--;
      }
      write_chars("</");
      write_chars(tag_stack[--tag_stack_size]);
      write_char('>');
    }
  }
  else { //not a close tag
    if(tag_stack_size >= MAX_STACK_DEPTH) {
      return FALSE;
    }
    if(strcmp(EM, name) == 0 || strcmp(STRONG, name) == 0 ||
       strcmp("s", name) == 0 || strcmp("u", name) == 0 ||
       strcmp("p", name) == 0 || strcmp("blockquote", name) == 0 ||
       strcmp("pre", name) == 0){
      if(tag_closed) {
        return FALSE;
      }
      if(num_attributes != 0) {
        return FALSE;
      }
      push_to_tag_stack(name, name_size);
      write_char('<');
      write_chars(name);
      write_char('>');
    }
    else if(strcmp("br", name) == 0) {
      if(num_attributes != 0) {
        return FALSE;
      }
      write_chars("<br/>");
    }
    else if(strcmp("a", name) == 0) {
      int href_pos;
      
      // printf("trying A tag\n");
      href_pos = attribute_find("href", attribute_keys,
        num_attributes);
      if(href_pos < 0) {
        return FALSE;
      }  
      if(!valid_url(attribute_values[href_pos])) {
        return FALSE;
      }
      
      write_chars("<a rel=\"nofollow\" target=\"_blank\" href=\"");
      write_chars(attribute_values[href_pos]);
      write_chars("\">");
      
      push_to_tag_stack("a", 1);
      anchors_in_stack++;
    }
    else if(strcmp("img", name) == 0) {
      int src_pos, alt_pos, width_pos, height_pos;
      
      src_pos = attribute_find("src", attribute_keys,
        num_attributes);
      alt_pos = attribute_find("alt", attribute_keys,
        num_attributes);
      width_pos = attribute_find("width", attribute_keys,
        num_attributes);
      height_pos = attribute_find("height", attribute_keys,
        num_attributes);
      if(src_pos < 0) {
        return FALSE;
      }
      
      if(!valid_url(attribute_values[src_pos])) {
        return FALSE;
      }
      
      write_chars("<img src=\"");
      write_chars(attribute_values[src_pos]);
      if (alt_pos >= 0){
        write_chars("\" alt=\"");
        write_chars(attribute_values[alt_pos]);
      }
      if (width_pos >= 0){
        write_chars("\" width=\"");
        write_chars(attribute_values[width_pos]);
      }
      if (height_pos >= 0){
        write_chars("\" height=\"");
        write_chars(attribute_values[height_pos]);
      }
      write_chars("\" class=\"escapedImg\"/>");
    }
    else {
      return FALSE;
    }
  }  
  return TRUE;
}

static int gr_tag_parse() {
  char *tag;
  char *attributes[MAX_ATTRIBUTES];
  int num_attributes = 0;
  char *name;
  int tag_size;
  int name_size;
  int i;
  char *id; // attribute[1]
  char *title;// attribute[2], defaulted to attr[0]
  tag = input + position;
  tag_size = (ahead_position - position) - 1;
  
  if(tag_size < 3) {
    return FALSE;
  }
  
  //read name
  for(name_size = 0; name_size < tag_size; name_size++) {
    if(tag[name_size] == ':') {
      break;
    }
  }
  name = ALLOCA_N(char, name_size + 1);
  strncpy(name, tag, name_size);
  name[name_size] = 0;
  downcasen(name, name_size);
  
  if(position + name_size + 1 > input_size) {
    printf("Error: position + name_size + 1 > input_size\n");
    return;
  }
  
  tag += name_size + 1;
  tag_size -= name_size + 1;
  while(tag_size > 0) {
    if (num_attributes == MAX_ATTRIBUTES) {
      return FALSE;
    }
    int attr_size;
    char *attr;
    for(attr_size = 0; attr_size < tag_size; attr_size++) {
      if(tag[attr_size] == '|') {
        break;
      }
    }
    attr = ALLOCA_N(char, attr_size + 1);
    strncpy(attr, tag, attr_size);
    attr[attr_size] = 0;
    tag += attr_size + 1;
    tag_size -= attr_size + 1;
    attributes[num_attributes++] = attr;
  }
  
  if (num_attributes < 1) {
    return FALSE;
  }
  
  if (num_attributes >= 2) {
    id = attributes[1];
  }
  else {
    id = 0;
  }
  
  if (num_attributes >= 3) {
    title = attributes[2];
  }
  else {
    title = attributes[0];
  }
  
  if(strcmp("b", name) == 0 || strcmp("book", name) == 0){
    if (id) {
      counting_cap = FALSE;
      write_chars("<a href=\"");
      if (absolute_url) {
        write_nchars(absolute_url, absolute_url_size);
      }
      write_chars("/book/show/");
      write_chars(id);
      write_char('.');
      write_urlitized_chars(title);
      write_chars("\" title=\"");
      write_escaped_chars(title);
      if (num_attributes >= 4) {
        write_chars(" by ");
        write_escaped_chars(attributes[3]);
      }
      write_chars("\">");
      counting_cap = TRUE;
      write_escaped_chars(attributes[0]);
      counting_cap = FALSE;
      write_chars("</a>");
      counting_cap = TRUE;
    }
    else {
      counting_cap = FALSE;
      write_chars("<a href=\"");
      if (absolute_url) {
        write_nchars(absolute_url, absolute_url_size);
      }
      write_chars("/search/search?q=");
      write_escaped_chars(attributes[0]);
      write_chars("\" title=\"");
      write_chars(title);
      write_chars("\">");
      counting_cap = TRUE;
      write_escaped_chars(attributes[0]);
      counting_cap = FALSE;
      write_chars("</a>");
      counting_cap = TRUE;
    }
  }
  else if(strcmp("bc", name) == 0){
    if (num_attributes >= 5) {
      counting_cap = FALSE;
      write_chars("<a href=\"");
      if (absolute_url) {
        write_nchars(absolute_url, absolute_url_size);
      }
      write_chars("/book/show/");
      write_chars(id);
      write_char('.');
      write_urlitized_chars(title);
      write_chars("\"><img src=\"");
      write_chars(attributes[4]);
      write_chars("\" title=\"");
      write_escaped_chars(title);
      if (num_attributes >= 4) {
        write_chars(" by ");
        write_escaped_chars(attributes[3]);
      }
      write_chars("\" alt=\"");
      write_escaped_chars(title);
      write_chars("\"/></a>");
      counting_cap = TRUE;
    }
    else {
      write_chars("[bookcover:");
      write_escaped_chars(attributes[0]);
      write_char(']');
    }
  }
  else if(strcmp("a", name) == 0 || strcmp("author", name) == 0){
    if (id) {
      counting_cap = FALSE;
      write_chars("<a href=\"");
      if (absolute_url) {
        write_nchars(absolute_url, absolute_url_size);
      }
      write_chars("/author/show/");
      write_chars(id);
      write_char('.');
      write_urlitized_chars(title);
      write_chars("\" title=\"");
      write_escaped_chars(title);
      write_chars("\">");
      counting_cap = TRUE;
      write_escaped_chars(attributes[0]);
      counting_cap = FALSE;
      write_chars("</a>");
      counting_cap = TRUE;
    }
    else {
      counting_cap = FALSE;
      write_chars("<a href=\"");
      if (absolute_url) {
        write_nchars(absolute_url, absolute_url_size);
      }
      write_chars("/search/search?q=");
      write_escaped_chars(attributes[0]);
      write_chars("\" title=\"");
      write_chars(title);
      write_chars("\">");
      counting_cap = TRUE;
      write_escaped_chars(attributes[0]);
      counting_cap = FALSE;
      write_chars("</a>");
      counting_cap = TRUE;
    }
  }
  else if(strcmp("ai", name) == 0){
    if (num_attributes >= 4) {
      counting_cap = FALSE;
      write_chars("<a href=\"");
      if (absolute_url) {
        write_nchars(absolute_url, absolute_url_size);
      }
      write_chars("/author/show/");
      write_chars(id);
      write_char('.');
      write_urlitized_chars(title);
      write_chars("\"><img src=\"");
      write_chars(attributes[3]);
      write_chars("\" title=\"");
      write_escaped_chars(title);
      write_chars("\" alt=\"");
      write_escaped_chars(title);
      write_chars("\"/></a>");
      counting_cap = TRUE;
    }
    else {
      write_chars("[authorimage:");
      write_escaped_chars(attributes[0]);
      write_char(']');
    }
  }
  else {
    return FALSE;
  }
  return TRUE;
}

static int html_read() {
  ahead_position = position;
  
  while(ahead_position < input_size) {
    switch(input[ahead_position++]) {
    case '<':
      return FALSE;
    case '>':
      return html_parse(position, ahead_position - 1);
    }
  }
  return FALSE;
}

static int gr_tag_read() {
  ahead_position = position;
  
  // reading name
  while(ahead_position < input_size) {
    switch(input[ahead_position++]) {
    case '[':
      return FALSE;
    case ']':
      return gr_tag_parse();
    }
  }
  
  return FALSE;
}

static int url_read() {
  char *url;
  char *url_downcase;
  char c;
  int i;
  int url_size;
  int has_http = FALSE;


  if(anchors_in_stack > 0) {
    return FALSE;
  }
  
  ahead_position = position;
  url_size = 0;

  while(ahead_position < input_size) {
    int done = FALSE;
    switch(input[ahead_position++]) {
    case '"':
    case '<':
    case '>':
      return FALSE;
    case ' ':
    case '\n':
    case '\t':
    case '(':
    case ')':
      ahead_position--;
      done = TRUE;
      break;
    }
    if(done) {
      break;
    }
  }
  url_size = 1 + ahead_position - position;
  
  if (url_size < 5) {
    return FALSE;
  }
  
  url = ALLOCA_N(char, url_size + 1);
  strncpy(url, input + position - 1, url_size);
  url[url_size] = 0;
  
  
  url_downcase = ALLOCA_N(char, url_size + 1);
  strncpy(url_downcase, url, url_size);
  downcasen(url_downcase, url_size);
  
  
  if (strncmp(url_downcase, "http://", 7) == 0) {
    has_http = TRUE;
  }
  else if (strncmp(url_downcase, "https://", 8) == 0) {
    has_http = TRUE;
  }
  else {
    //try and decide if the its a url without 'http' in front
    int has_www = FALSE;
    int last_dot = -1;
    int done = TRUE;
    int tld_size;
    
    //does it start with www.?
    if (strncmp(url_downcase, "www.", 4) == 0) {
      has_www = TRUE;
      i = 4;
      last_dot = 3;
    }
    else {
      i = 0;
    }
    
    // see if it starts with a properly formed domain name
    for(; i < url_size; i++) {
      c = url_downcase[i];
      if (c == '.') {
        //starting with a period is invalid
        if(i == 0) {
          return FALSE;
        }
        
        //two periods in a row is invalid!
        if(last_dot + 1 == i){
          return FALSE;
        }
        last_dot = i;
      }
      else if (c == '/') {
        // a slash means we're no longer reading a domain name
        break;
      }
      else if ((c >= 'a' && c <= 'z') || c == '-' ||
               (c >= '0' && c <= '9')) {
        // valid domain name characters
      }
      else {
        // domains must be made up of those other characters
        return FALSE;
      }
    }  
    
    if(last_dot == -1) { // no periods were found
      return FALSE;
    }
    tld_size = (i - last_dot) - 1;
    if (has_www) {
      if (tld_size < 2){
        return FALSE;
      }
    }
    else {
      char *tld;
      if (tld_size != 3){
        return FALSE;
      }
      tld = url + last_dot + 1;
      if(strncmp(tld, "com", 3) != 0 &&
         strncmp(tld, "net", 3) != 0 &&
         strncmp(tld, "org", 3) != 0 &&
         strncmp(tld, "gov", 3) != 0){
        // not a tld we autogenerate for!
        return FALSE;
      }
    }
    
  }
  
  //OK, now its probably ok to generate the url
  counting_cap = FALSE;
  write_chars("<a rel=\"nofollow\" target=\"_blank\" href=\"");
  if (!has_http) {
    write_chars("http://");
  }
  write_chars(url);
  if (url_size > MAX_URL_PRINT_SIZE) {
    write_chars("\" title=\"");
    write_chars(url);
  }
  write_chars("\">");
  counting_cap = TRUE;
  for(i = 0; i < MAX_URL_PRINT_SIZE && i < url_size; i++) {
    switch(c = url[i]) {
    case '&':
      write_chars("&amp;");
      break;
    default:
      write_char(c);
      break;
    }
  }
  if(i < url_size) {
    write_chars("...");
  }
  counting_cap = FALSE;
  write_chars("</a>");
  counting_cap = TRUE;
  return TRUE;
}


static int amp_read() {
  int amp_escape_count = 0;
  int poundsign = FALSE;
  ahead_position = position;
  
  // reading name
  while(ahead_position < input_size) {
    char c;
    c = input[ahead_position++];
    if (c == '#') {
      if(amp_escape_count == 0) {
        poundsign = TRUE;
      }
      else {
        return FALSE;
      }
    }
    else if(c >= '0' && c <= '9') {
    }
    else if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      if (poundsign) {
        return FALSE;
      }
    }
    else if (c == ';') {
      if (amp_escape_count >= 2){
        write_nchars(input + position - 1, amp_escape_count + 2);
        return TRUE;
      }
      else {
        return FALSE;
      }
    }
    else {
      return FALSE;
    }
    
    if(amp_escape_count++ > 6) {
      return FALSE; 
    }
  }
  
  return FALSE;
}


static VALUE t_parse(VALUE self, VALUE r_string, VALUE r_cap, VALUE r_cap_string) {
  char c;
  int try_url = TRUE;
  int cap_at;
  char * cap_string;
  VALUE result;
  tag_stack_size = 0;
  anchors_in_stack = 0;
  counting_cap = TRUE;
  cap_count = 0;
  writing_utf_8 = 0;
  position = ahead_position = 0;
  
  //de-ruby all the inputs!
  r_string = StringValue(r_string);
  input_size = RSTRING(r_string)->len;
  input = RSTRING(r_string)->ptr;
  
  if (NIL_P(r_cap)) {
    cap_at = 0;
  }
  else {
    cap_at = NUM2INT(r_cap);
  }
  
  cap_string = StringValuePtr(r_cap_string);
  
  max_output_size = input_size * 10 + 128;
  output = ALLOCA_N(char, max_output_size);
  output_size = 0;
  
  tag_stack = ALLOCA_N(char *, MAX_STACK_DEPTH);
  
  while(position < input_size) {
    
    if(cap_at && cap_count >= cap_at) {
      write_chars(cap_string);
      break;
    }
    switch(c = input[position++]) {
    case '&':
      counting_cap = FALSE;
      if(amp_read()) {
        position = ahead_position;
      } 
      else {
        write_chars("&amp;");
      }
      counting_cap = TRUE;
      try_url = FALSE;
      cap_count++;
      break;
    case '>':
      try_url = FALSE;
      write_chars("&gt;");
      break;
    case '<':
      counting_cap = FALSE;
      if(html_read()) {
        position = ahead_position;
        try_url = TRUE;
        counting_cap = TRUE;
      }
      else {
        counting_cap = TRUE;
        write_chars("&lt;");
        try_url = FALSE;
      }
      break;
    case '[':
      counting_cap = FALSE;
      if(gr_tag_read()) {
        position = ahead_position;
        try_url = TRUE;
        counting_cap = TRUE;
      }
      else {
        try_url = FALSE;
        counting_cap = TRUE;
        write_char('[');
      }
      break;
    case '"':
      try_url = FALSE;
      write_chars("&quot;");
      break;
    case '\n':
      write_chars("<br/>");
      try_url = TRUE;
      break;
    case ' ':
    case '\t':
    case '(':
    case ')':
      write_char(c);
      try_url = TRUE;
      break;
    default:
      if(try_url){
        if(url_read()) {
          position = ahead_position;
        }
        else {
          write_char(c);
        }
        try_url = FALSE;
      }
      else {
        write_char(c);
      }
      break;
    }
  }
  while(tag_stack_size > 0) {
    char *item = tag_stack[--tag_stack_size];
    write_chars("</");
    write_chars(item);
    free(item);
    write_char('>');
  }
  return rb_str_new(output, output_size);
}


static VALUE t_set_absolute_url(VALUE self, VALUE r_string) {
  int new_size;
  char *new_url;
  r_string = StringValue(r_string);
  absolute_url_size = RSTRING(r_string)->len;
  new_url = RSTRING(r_string)->ptr;
  if (absolute_url) {
    free(absolute_url);
  }
  absolute_url = malloc(absolute_url_size);
  strncpy(absolute_url, new_url, absolute_url_size);
  return r_string;
}


VALUE cTest;

void Init_gr_string_escape() {
  cTest = rb_define_class("GrStringEscape", rb_cObject);
  rb_define_method(cTest, "parse", t_parse, 3);
  rb_define_method(cTest, "set_absolute_url", t_set_absolute_url, 1);
  id_push = rb_intern("push");
}
