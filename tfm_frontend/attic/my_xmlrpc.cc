///////////////////////////////////////////////////////////////////////////////
//       P.Murat: use xmlrpc.c as a template
///////////////////////////////////////////////////////////////////////////////

/* Make an XML-RPC call.

   User specifies details of the call on the command line.
   We print the result on Standard Output.

   Example:

     $ xmlrpc http://localhost:8080/RPC2 sample.add i/3 i/5
     Result:
       Integer: 8

     $ xmlrpc localhost:8080 sample.add i/3 i/5
     Result:
       Integer: 8
*/

#define _XOPEN_SOURCE 600  /* Make sure strdup() is in <string.h> */

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <string>
#include <errno.h>
#include <assert.h>

#include "xmlrpc_config.h"  /* information about this build environment */
// #include "bool.h"
// #include "int.h"
#include "mallocvar.h"
// #include "girstring.h"
#include "casprintf.h"
#include "string_parser.h"
#include "cmdline_parser.h"
// #include "dumpvalue.h"

#include "xmlrpc-c/base.h"
#include "xmlrpc-c/client.h"
#include "string_int.h"

#include "tfm/my_xmlrpc.hh"

#define NAME "xmlrpc command line program"
#define VERSION "1.0"

struct cmdlineInfo {
  const char*   url;
  const char*   username;
  const char*   password;
  const char*   methodName;
  unsigned int  paramCount;
                                        /* Array of parameters, in order.  Has 'paramCount' entries.
                                           Each of these is as it appears on the command line, e.g. "i/4"
                                        */
  const char**  params;
                                        /* Name of XML transport he wants to use.  NULL if he has no
                                           preference.
                                        */
  const char *  transport;
                                        /* "network interface" parameter for the Curl transport.  (Not
                                           valid if 'transport' names a non-Curl transport).
                                        */
  const char *  curlinterface;
  xmlrpc_bool   curlnoverifypeer;
  xmlrpc_bool   curlnoverifyhost;
  const char *  curluseragent;
  bool          xmlsizelimitSpec;
  unsigned int  xmlsizelimit;
};

struct _xmlrpc_value;

void dumpValue(const char* prefix, struct _xmlrpc_value* valueP);

//-----------------------------------------------------------------------------
static void dumpResult(xmlrpc_value * const resultP) {
  printf("Result:");
  dumpValue("", resultP);
}

//-----------------------------------------------------------------------------
static __inline__ void newVsnprintf(char *       const buffer,
                                    size_t       const bufferSize,
                                    const char * const fmt,
                                    va_list            varargs,
                                    size_t *     const formattedSizeP) {
/*----------------------------------------------------------------------------
   This is vsnprintf() with the new behavior, where not fitting in the buffer
   is not a failure.

   Unfortunately, we can't practically return the size of the formatted string
   if the C library has old vsnprintf() and the formatted string doesn't fit
   in the buffer, so in that case we just return something larger than the
   buffer.
-----------------------------------------------------------------------------*/
    if (bufferSize > INT_MAX/2) {
        /* There's a danger we won't be able to coerce the return value
           of XMLRPC_VSNPRINTF to an integer (which we have to do because,
           while for POSIX its return value is ssize_t, on Windows it is int),
           or return double the buffer size.
        */
        *formattedSizeP = 0;
    } else {
        int rc;

        rc = XMLRPC_VSNPRINTF(buffer, bufferSize, fmt, varargs);

        if (rc < 0) {
            /* We have old vsnprintf() (or Windows) and the formatted value
               doesn't fit in the buffer, but we don't know how big a buffer it
               needs.
            */
            *formattedSizeP = bufferSize * 2;
        } else {
            /* Either the string fits in the buffer or we have new vsnprintf()
               which tells us how big the string is regardless.
            */
            *formattedSizeP = rc;
        }
    }
}



static __inline__ int simpleVasprintf(char **      const resultP,
                                      const char * const fmt,
                                      va_list            varargs) {
/*----------------------------------------------------------------------------
   This is a poor man's implementation of vasprintf(), of GNU fame.
-----------------------------------------------------------------------------*/
  int    retval;
  char * buffer;
  size_t bufferSize;
  bool   outOfMemory;

  for (buffer = NULL, bufferSize = 4096, outOfMemory = false; !buffer && !outOfMemory; ) {

    buffer = (char*) malloc(bufferSize);
    if (!buffer)
      outOfMemory = true;
    else {
      size_t bytesNeeded;
      newVsnprintf(buffer, bufferSize, fmt, varargs, &bytesNeeded);
      if (bytesNeeded > bufferSize) {
        free(buffer);
        buffer = NULL;
        bufferSize = bytesNeeded;
      }
    }
  }

  if (outOfMemory) retval = -1;
  else {
    retval = strlen(buffer);
    *resultP = buffer;
  }
  return retval;
}


const char * const strsol = "[Insufficient memory to build string]";

//-----------------------------------------------------------------------------
void cvasprintf(const char ** retvalP, const char *  fmt, va_list varargs) {
  char* string;
  int   rc;

#if HAVE_ASPRINTF
    rc = vasprintf(&string, fmt, varargs);
#else
    rc = simpleVasprintf(&string, fmt, varargs);
#endif

    if (rc < 0)
        *retvalP = strsol;
    else
        *retvalP = string;
}


// void GNU_PRINTF_ATTR(2,3)
void casprintf(const char ** retvalP, const char * fmt, ...) {

    va_list varargs;  /* mysterious structure used by variable arg facility */

    va_start(varargs, fmt); /* start up the mysterious variable arg facility */

    cvasprintf(retvalP, fmt, varargs);

    va_end(varargs);
}


//-----------------------------------------------------------------------------
void strfree(const char* string) {
  if (string != strsol) free((void *)string);
}

static void die_if_fault_occurred (const xmlrpc_env* envP) {
  if (envP->fault_occurred) {
    fprintf(stderr, "Failed.  %s\n", envP->fault_string);
    exit(1);
  }
}

// static void GNU_PRINTF_ATTR(2,3)
static void setError(const xmlrpc_env* envP, const char* format, ...) {
  va_list     args;
  const char* faultString;
  
  va_start(args,format);
  
  cvasprintf(&faultString, format, args);
  va_end(args);
  
  xmlrpc_env_set_fault((xmlrpc_env*) envP, XMLRPC_INTERNAL_ERROR, faultString);
  
  strfree(faultString);
}


//-----------------------------------------------------------------------------
static void processArguments(xmlrpc_env* envP, cmdlineParser cp, cmdlineInfo* cmdlineP) {

  if (cmd_argumentCount(cp) < 2) {
    std::string m = "Not enough arguments.  Need at least a URL and method name.";
    setError(envP, m.data());
  }
  else {
    cmdlineP->url        = cmd_getArgument(cp, 0);
    cmdlineP->methodName = cmd_getArgument(cp, 1);
    cmdlineP->paramCount = cmd_argumentCount(cp) - 2;

    // MALLOCARRAY_NOFAIL(cmdlineP->params, cmdlineP->paramCount);

    mallocProduct((void**) &cmdlineP->params, cmdlineP->paramCount, sizeof(cmdlineP->params[0]));

    for (uint i=0; i < cmdlineP->paramCount; ++i)
      cmdlineP->params[i] = cmd_getArgument(cp, i+2);
  }
}

static void chooseTransport(xmlrpc_env *  const envP ATTR_UNUSED,
                            cmdlineParser const cp,
                            const char ** const transportPP) {

  const char * transportOpt = cmd_getOptionValueString(cp, "transport");

  if (transportOpt) {
    *transportPP = transportOpt;
  } 
  else {
    if (cmd_optionIsPresent(cp, "curlinterface") ||
        cmd_optionIsPresent(cp, "curlnoverifypeer") ||
        cmd_optionIsPresent(cp, "curlnoverifyhost") ||
        cmd_optionIsPresent(cp, "curluseragent"))

      *transportPP = strdup("curl");
    else
      *transportPP = NULL;
  }
}

static void parseCommandLine(xmlrpc_env *         const envP,
                             int                  const argc,
                             const char **        const argv,
                             struct cmdlineInfo * const cmdlineP) {

  cmdlineParser const cp = cmd_createOptionParser();

  const char * error;

  cmd_defineOption(cp, "transport",        OPTTYPE_STRING);
  cmd_defineOption(cp, "username",         OPTTYPE_STRING);
  cmd_defineOption(cp, "password",         OPTTYPE_STRING);
  cmd_defineOption(cp, "curlinterface",    OPTTYPE_STRING);
  cmd_defineOption(cp, "curlnoverifypeer", OPTTYPE_STRING);
  cmd_defineOption(cp, "curlnoverifyhost", OPTTYPE_STRING);
  cmd_defineOption(cp, "curluseragent",    OPTTYPE_STRING);
  cmd_defineOption(cp, "xmlsizelimit",     OPTTYPE_BINUINT);

  cmd_processOptions(cp, argc, argv, &error);

  if (error) {
    std::string format = "Command syntax error.  %s";
    setError(envP, format.data(),error);
    strfree(error);
  } 
  else {
    if (cmd_optionIsPresent(cp, "xmlsizelimit")) {
      cmdlineP->xmlsizelimitSpec = true;
      cmdlineP->xmlsizelimit =
        cmd_getOptionValueUint(cp, "xmlsizelimit");
    } 
    else {
      cmdlineP->xmlsizelimitSpec = false;
    }
    cmdlineP->username  = cmd_getOptionValueString(cp, "username");
    cmdlineP->password  = cmd_getOptionValueString(cp, "password");

    if (cmdlineP->username && !cmdlineP->password) {
      std::string msg = "When you specify -username, you must also specify -password.";
      setError(envP, msg.data());
    }
    else {
      chooseTransport(envP, cp, &cmdlineP->transport);

      cmdlineP->curlinterface =
        cmd_getOptionValueString(cp, "curlinterface");
      cmdlineP->curlnoverifypeer =
        cmd_optionIsPresent(cp, "curlnoverifypeer");
      cmdlineP->curlnoverifyhost =
        cmd_optionIsPresent(cp, "curlnoverifyhost");
      cmdlineP->curluseragent =
        cmd_getOptionValueString(cp, "curluseragent");

      if ((!cmdlineP->transport || (strcmp(cmdlineP->transport, "curl") != 0)) 
          &&
          (cmdlineP->curlinterface    || cmdlineP->curlnoverifypeer || 
           cmdlineP->curlnoverifyhost || cmdlineP->curluseragent)      ) {

        std::string msg = "You may not specify a Curl transport option unless you also specify -transport=curl";
        setError(envP, msg.data());
      }

      processArguments(envP, cp, cmdlineP);
    }
  }
  cmd_destroyOptionParser(cp);
}

static void freeCmdline(struct cmdlineInfo const cmdline) {

  strfree(cmdline.url);
  strfree(cmdline.methodName);
  if (cmdline.transport)     strfree(cmdline.transport);
  if (cmdline.curlinterface) strfree(cmdline.curlinterface);
  if (cmdline.curluseragent) strfree(cmdline.curluseragent);
  if (cmdline.username)      strfree(cmdline.username);
  if (cmdline.password)      strfree(cmdline.password);

  for (uint i = 0; i < cmdline.paramCount; ++i) {
    strfree(cmdline.params[i]);
  }
}

static void computeUrl(const char* const urlArg, const char** const urlP) {
  if (strstr(urlArg, "://") != 0) casprintf(urlP, "%s", urlArg);
  else                            casprintf(urlP, "http://%s/RPC2", urlArg);
}

enum TokenType {COMMA, COLON, LEFTPAREN, RIGHTPAREN,
                LEFTBRACE, RIGHTBRACE, END};

static const char* tokenTypeName(enum TokenType const type) {

  switch (type) {
  case COMMA:      return "comma";
  case COLON:      return "colon";
  case LEFTPAREN:  return "left parenthesis";
  case RIGHTPAREN: return "right parenthesis";
  case LEFTBRACE:  return "left brace";
  case RIGHTBRACE: return "right brace";
  case END:        return "end of string";
  }
  return NULL; /* defeat bogus compiler warning */
}

static void getDelimiter(xmlrpc_env *     const envP,
                         const char **    const cursorP,
                         enum TokenType * const typeP) {

  const char * cursor;
  enum TokenType tokenType;

  cursor = *cursorP;

  switch (*cursor) {
  case ',': tokenType = COMMA; break;
  case ':': tokenType = COLON; break;
  case '(': tokenType = LEFTPAREN; break;
  case ')': tokenType = RIGHTPAREN; break;
  case '{': tokenType = LEFTBRACE; break;
  case '}': tokenType = RIGHTBRACE; break;
  case '\0': tokenType = END; break;
  default:
    std::string format = "Unrecognized delimiter character '%c'";
    setError(envP, format.data(), *cursor);
  }

  if (!envP->fault_occurred && tokenType != END)
    ++cursor;

  *cursorP = cursor;
  *typeP = tokenType;
}

static void getCdata(xmlrpc_env *  const envP,
                     const char ** const cursorP,
                     const char ** const cdataP) {

  size_t const cdataSizeBound = strlen(*cursorP) + 1;

  char * text;

  // MALLOCARRAY(text, cdataSizeBound);
  mallocProduct((void**) &text, cdataSizeBound, sizeof(text[0]));

  if (text == NULL) {
    std::string format = "Failed to allocate a buffer of size %u to compute cdata";
    setError(envP, format.data(), (unsigned)cdataSizeBound);
  }
  else {
    unsigned int textCursor;
    bool         end;
    const char*  cursor;

    cursor = *cursorP;  /* initial value */

    for (textCursor = 0, end = false; !end; ) {
      switch (*cursor) {
      case ',':
      case ':':
      case '(':
      case ')':
      case '{':
      case '}':
      case '\0':
        end = true;
        break;
      case '\\': {
        ++cursor;  // skip backslash escape character
        if (!*cursor) {
          std::string msg = "Nothing after escape character ('\\')";
          setError(envP,msg.data());
        }
        else
          text[textCursor++] = *cursor++;
      }; break;
      default:
        text[textCursor++]= *cursor++;
      }
      assert(textCursor <= cdataSizeBound);
    }
    text[textCursor++] = '\0';

    assert(textCursor <= cdataSizeBound);

    *cdataP  = text;
    *cursorP = cursor;
  }
}

static void buildValue(xmlrpc_env *    const envP,
                       const char **   const cursorP,
                       xmlrpc_value ** const valuePP);  // for recursion

static void buildString(xmlrpc_env *    const envP,
                        const char *    const valueString,
                        xmlrpc_value ** const paramPP) {

    *paramPP = xmlrpc_string_new(envP, valueString);
}

static void interpretHex(xmlrpc_env *    const envP,
                         const char *    const valueString,
                         size_t          const valueStringSize,
                         unsigned char * const byteString) {
  size_t bsCursor;
  size_t strCursor;

  for (strCursor = 0, bsCursor = 0;
       strCursor < valueStringSize && !envP->fault_occurred;
       ) {
    int rc;

    rc = sscanf(&valueString[strCursor], "%2hhx",
                &byteString[bsCursor++]);

    if (rc != 1)
      xmlrpc_faultf(envP, "Invalid hex data '%s'",
                    &valueString[strCursor]);
    else
      strCursor += 2;
  }
}

static void buildBytestring(xmlrpc_env*    const envP,
                            const char*    const valueString,
                            xmlrpc_value** const paramPP) {

  size_t const valueStringSize = strlen(valueString);

  if (valueStringSize / 2 * 2 != valueStringSize)
    xmlrpc_faultf(envP, "Hexadecimal text is not an even "
                  "number of characters (it is %u characters)",
                  (unsigned)strlen(valueString));
  else {
    size_t const byteStringSize = strlen(valueString)/2;

    unsigned char * byteString;

    // MALLOCARRAY(byteString, byteStringSize);
    mallocProduct((void**) &byteString, byteStringSize, sizeof(char));

    if (byteString == NULL)
      xmlrpc_faultf(envP, "Failed to allocate %u-byte buffer",
                    (unsigned)byteStringSize);
    else {
      interpretHex(envP, valueString, valueStringSize, byteString);

      if (!envP->fault_occurred)
        *paramPP = xmlrpc_base64_new(envP, byteStringSize, byteString);

      free(byteString);
    }
  }
}

//-----------------------------------------------------------------------------
static void buildInt(xmlrpc_env* envP, const char* valueString, xmlrpc_value** paramPP) {

  if (strlen(valueString) < 1) {
    std::string msg = "Integer argument has nothing after the 'i/'";
    setError(envP, msg.data());
  }
  else {
    int value;
    const char * error;

    interpretInt(valueString, &value, &error);

    if (error) {
      std::string fmt = "'%s' is not a valid 32-bit integer.  %s";
      setError(envP,fmt.data(),valueString, error);
      strfree(error);
    } 
    else {
      *paramPP = xmlrpc_int_new(envP, value);
    }
  }
}

static void buildBool(xmlrpc_env *    const envP,
                      const char *    const valueString,
                      xmlrpc_value ** const paramPP) {

  if ((strcmp(valueString, "t") == 0) || (strcmp(valueString, "true") == 0))
    *paramPP = xmlrpc_bool_new(envP, true);
  else if ((strcmp(valueString, "f") == 0) || (strcmp(valueString, "false") == 0))
    *paramPP = xmlrpc_bool_new(envP, false);
  else {
    std::string fmt = "Boolean argument has unrecognized value '%s'. Recognized values are 't', 'f', 'true', and 'false'.";
    setError(envP, fmt.data(), valueString);
  }
}

static void buildDouble(xmlrpc_env *    const envP,
                        const char *    const valueString,
                        xmlrpc_value ** const paramPP) {

  if (strlen(valueString) < 1) {
    std::string msg = "\"Double\" argument has nothing after the 'd/'";
    setError(envP,msg.data());
  }
  else {
    double value;
    char * tailptr;

    value = strtod(valueString, &tailptr);

    if (*tailptr != '\0') {
      std::string fmt = "\"Double\" argument has non-decimal crap in it: '%s'";
      setError(envP,fmt.data(),tailptr);
    }
    else
      *paramPP = xmlrpc_double_new(envP, value);
  }
}

static void buildNil(xmlrpc_env *    const envP,
                     const char *    const valueString,
                     xmlrpc_value ** const paramPP) {

  if (strlen(valueString) > 0) {
    std::string msg = "Nil argument has something after the 'n/'";
    setError(envP, msg.data());
  }
  else {
    *paramPP = xmlrpc_nil_new(envP);
  }
}

static void buildI8(xmlrpc_env *    const envP,
                    const char *    const valueString,
                    xmlrpc_value ** const paramPP) {

  if (strlen(valueString) < 1)
    setError(envP, "Integer argument has nothing after the 'I/'");
  else {
    int64_t value;
    const char * error;

    interpretLl(valueString, &value, &error);

    if (error) {
      setError(envP, "'%s' is not a valid 64-bit integer.  %s",
               valueString, error);
      strfree(error);
    } else
      *paramPP = xmlrpc_i8_new(envP, value);
  }
}

static void addArrayItem(xmlrpc_env *   const envP,
                         const char **  const cursorP,
                         xmlrpc_value * const arrayP,
                         bool *         const endP) {

  xmlrpc_value * itemP;

  buildValue(envP, cursorP, &itemP);

  if (!envP->fault_occurred) {
    xmlrpc_array_append_item(envP, arrayP, itemP);

    if (!envP->fault_occurred) {
      enum TokenType delim;

      getDelimiter(envP, cursorP, &delim);

      if (!envP->fault_occurred) {
        switch (delim) {
        case COMMA: break;
        case RIGHTPAREN: *endP = true; break;
        default:
          setError(envP, "Array specifier has garbage where "
                   "there should be a comma "
                   "(element separator) "
                   "or close parenthesis "
                   "(marking the end of the element list)");
        }
      }
    }
    xmlrpc_DECREF(itemP);
  }
}

static void buildArray(xmlrpc_env *    const envP,
                       const char **   const cursorP,
                       xmlrpc_value ** const valuePP) {

  enum TokenType tokenType;

  getDelimiter(envP, cursorP, &tokenType);

  if (!envP->fault_occurred) {
    if (tokenType != LEFTPAREN)
      setError(envP, "Array specifier value starts with %s instead of "
               "left parenthesis", tokenTypeName(tokenType));
    else {
      xmlrpc_value * const arrayP = xmlrpc_array_new(envP);

      if (!envP->fault_occurred) {
        bool end;  /* We've reached the end of the array elements */
        for (end = false; !end && !envP->fault_occurred; )
          addArrayItem(envP, cursorP, arrayP, &end);

        if (envP->fault_occurred)
          xmlrpc_DECREF(arrayP);
        else
          *valuePP = arrayP;
      }
    }
  }
}

static void addStructMember(xmlrpc_env *   const envP,
                            const char **  const cursorP,
                            xmlrpc_value * const structP,
                            bool *         const endP) {
  const char * key;

  getCdata(envP, cursorP, &key);

  if (!envP->fault_occurred) {
    enum TokenType delim;

    getDelimiter(envP, cursorP, &delim);

    if (!envP->fault_occurred) {
      if (delim != COLON)
        setError(envP, "Something other than a colon follows the "
                 "key value '%s' in a structure member.", key);
      else {
        xmlrpc_value * valueP;

        buildValue(envP, cursorP, &valueP);

        if (!envP->fault_occurred) {
          xmlrpc_struct_set_value(envP, structP, key, valueP);

          if (!envP->fault_occurred) {
            enum TokenType delim;

            getDelimiter(envP, cursorP, &delim);

            if (!envP->fault_occurred) {
              switch (delim) {
              case COMMA: break;
              case RIGHTBRACE: *endP = true; break;
              default:
                setError(envP, "Struct specifier "
                         "has garbage where "
                         "there should be a comma "
                         "(member separator) "
                         "or close brace "
                         "(marking the end of the "
                         "member list)");
              }
            }
          }
          xmlrpc_DECREF(valueP);
        }
      }
    }
    strfree(key);
  }
}

static void buildStruct(xmlrpc_env*    const envP,
                        const char**   const cursorP,
                        xmlrpc_value** const valuePP) {

  enum TokenType tokenType;

  getDelimiter(envP, cursorP, &tokenType);

  if (!envP->fault_occurred) {
    if (tokenType != LEFTBRACE)
      setError(envP, "Struct specifier value starts with %s instead of "
               "left brace", tokenTypeName(tokenType));
    else {
      xmlrpc_value * const structP = xmlrpc_struct_new(envP);

      if (!envP->fault_occurred) {
        bool end;  /* We've reached the end of the struct members */
        for (end = false; !end && !envP->fault_occurred; )
          addStructMember(envP, cursorP, structP, &end);

        if (envP->fault_occurred)
          xmlrpc_DECREF(structP);
        else
          *valuePP = structP;
      }
    }
  }
}

static void buildValue(xmlrpc_env *    const envP,
                       const char **   const cursorP,
                       xmlrpc_value ** const valuePP) {
/*----------------------------------------------------------------------------
   Parse the text at *cursorP as a specification of an XML-RPC value
   (e.g. "i/4" or "array/(i/0,i/2,i/2)")

   Stop parsing at the end of the specification of one value.  Advance
   *cursorP to that spot.
-----------------------------------------------------------------------------*/
  const char * cdata;

  getCdata(envP, cursorP, &cdata);
  /* This should get e.g. "i/492" or "hello" or "array/" */

  if (!envP->fault_occurred) {
    if (strlen(cdata) == 0)
      setError(envP, "Expected value type specifier such as 'i/' or "
               "'array/' but found '%s'", *cursorP);

    if (xmlrpc_strneq(cdata, "s/", 2))
      buildString(envP, &cdata[2], valuePP);
    else if (xmlrpc_strneq(cdata, "h/", 2))
      buildBytestring(envP, &cdata[2], valuePP);
    else if (xmlrpc_strneq(cdata, "i/", 2))
      buildInt(envP, &cdata[2], valuePP);
    else if (xmlrpc_strneq(cdata, "I/", 2))
      buildI8(envP, &cdata[2], valuePP);
    else if (xmlrpc_strneq(cdata, "d/", 2))
      buildDouble(envP, &cdata[2], valuePP);
    else if (xmlrpc_strneq(cdata, "b/", 2))
      buildBool(envP, &cdata[2], valuePP);
    else if (xmlrpc_strneq(cdata, "n/", 2))
      buildNil(envP, &cdata[2], valuePP);
    else if (xmlrpc_strneq(cdata, "array/", 6)) {
      if (cdata[6] != '\0')
        setError(envP, "Junk after 'array/' instead of "
                 "left parenthesis: '%s'", &cdata[6]);
      else
        buildArray(envP, cursorP, valuePP);
    } else if (xmlrpc_strneq(cdata, "struct/", 7)) {
      if (cdata[7] != '\0')
        setError(envP, "Junk after 'struct/' instead of "
                 "left brace: '%s'", &cdata[7]);
      else
        buildStruct(envP, cursorP, valuePP);
    } else {
      /* It's not in normal type/value format, so we take it to be
         the shortcut string notation
      */
      buildString(envP, cdata, valuePP);
    }
    strfree(cdata);
  }
}

static void computeParam(xmlrpc_env*    const envP,
                         const char*    const paramArg,
                         xmlrpc_value** const paramPP) {

  const char * cursor;

  cursor = &paramArg[0];

  buildValue(envP, &cursor, paramPP);

  if (!envP->fault_occurred) {
    if (*cursor != '\0')
      setError(envP, "Junk after parameter specification: '%s'", cursor);
  }
}

static void computeParamArray(xmlrpc_env*    const envP,
                              unsigned int   const paramCount,
                              const char**   const params,
                              xmlrpc_value** const paramArrayPP) {

  xmlrpc_value * paramArrayP;

  paramArrayP = xmlrpc_array_new(envP);

  for (uint i = 0; i < paramCount && !envP->fault_occurred; ++i) {
    xmlrpc_value * paramP;
    xmlrpc_env paramEnv;

    xmlrpc_env_init(&paramEnv);

    computeParam(&paramEnv, params[i], &paramP);

    if (!paramEnv.fault_occurred) {
      xmlrpc_array_append_item(&paramEnv, paramArrayP, paramP);

      xmlrpc_DECREF(paramP);
    }
    if (paramEnv.fault_occurred)
      setError(envP, "Invalid specification of parameter %u "
               "(starting at zero).  %s", i, paramEnv.fault_string);

    xmlrpc_env_clean(&paramEnv);
  }
  *paramArrayPP = paramArrayP;
}

static void callWithClient(xmlrpc_env *               const envP,
                           const xmlrpc_server_info * const serverInfoP,
                           const char *               const methodName,
                           xmlrpc_value *             const paramArrayP,
                           xmlrpc_value **            const resultPP) {
  xmlrpc_env env;
  xmlrpc_env_init(&env);
  *resultPP = xmlrpc_client_call_server_params(
                                               &env, serverInfoP, methodName, paramArrayP);

  if (env.fault_occurred)
    xmlrpc_faultf(envP, "Call failed.  %s.  (XML-RPC fault code %d)",
                  env.fault_string, env.fault_code);
  xmlrpc_env_clean(&env);
}

static void doCall(xmlrpc_env *               const envP,
                   const char *               const transport,
                   const char *               const curlinterface,
                   xmlrpc_bool                const curlnoverifypeer,
                   xmlrpc_bool                const curlnoverifyhost,
                   const char *               const curluseragent,
                   const xmlrpc_server_info * const serverInfoP,
                   const char *               const methodName,
                   xmlrpc_value *             const paramArrayP,
                   xmlrpc_value **            const resultPP) {
  
  struct xmlrpc_clientparms clientparms;

  XMLRPC_ASSERT(xmlrpc_value_type(paramArrayP) == XMLRPC_TYPE_ARRAY);

  clientparms.transport = transport;

  if (transport && (strcmp(transport, "curl") == 0)) {
    xmlrpc_curl_xportparms* curlXportParmsP = (xmlrpc_curl_xportparms*) malloc(sizeof(xmlrpc_curl_xportparms));
    // MALLOCVAR(curlXportParmsP);

    curlXportParmsP->network_interface = curlinterface;
    curlXportParmsP->no_ssl_verifypeer = curlnoverifypeer;
    curlXportParmsP->no_ssl_verifyhost = curlnoverifyhost;
    curlXportParmsP->user_agent        = curluseragent;

    clientparms.transportparmsP    = curlXportParmsP;
    clientparms.transportparm_size = XMLRPC_CXPSIZE(user_agent);
  } else {
    clientparms.transportparmsP = NULL;
    clientparms.transportparm_size = 0;
  }
  xmlrpc_client_init2(envP, XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION,
                      &clientparms, XMLRPC_CPSIZE(transportparm_size));
  if (!envP->fault_occurred) {
    callWithClient(envP, serverInfoP, methodName, paramArrayP, resultPP);

    xmlrpc_client_cleanup();
  }
  if (clientparms.transportparmsP)
    free((void*)clientparms.transportparmsP);
}

static void createServerInfo(xmlrpc_env *          envP,
                             const char *          serverUrl,
                             const char *          userName,
                             const char *          password,
                             xmlrpc_server_info ** serverInfoPP) {

  xmlrpc_server_info * serverInfoP;

  serverInfoP = xmlrpc_server_info_new(envP, serverUrl);
  if (!envP->fault_occurred) {
    if (userName) {
      xmlrpc_server_info_set_basic_auth(
                                        envP, serverInfoP, userName, password);
    }
  }
  *serverInfoPP = serverInfoP;
}

//-----------------------------------------------------------------------------
int my_xmlrpc(int argc, const char** argv, std::string& Res) {

  struct cmdlineInfo  cmdline;
  xmlrpc_env          env;
  xmlrpc_value*       paramArrayP;
  xmlrpc_value*       resultP;
  const char*         url;
  xmlrpc_server_info* serverInfoP;

  xmlrpc_env_init(&env);

  parseCommandLine(&env, argc, argv, &cmdline);
  die_if_fault_occurred(&env);

  if (cmdline.xmlsizelimitSpec)
    xmlrpc_limit_set(XMLRPC_XML_SIZE_LIMIT_ID, cmdline.xmlsizelimit);

  computeUrl(cmdline.url, &url);

  computeParamArray(&env, cmdline.paramCount, cmdline.params, &paramArrayP);
  die_if_fault_occurred(&env);

  createServerInfo(&env, url, cmdline.username, cmdline.password,
                   &serverInfoP);
  die_if_fault_occurred(&env);

  doCall(&env, cmdline.transport, cmdline.curlinterface,
         cmdline.curlnoverifypeer, cmdline.curlnoverifyhost,
         cmdline.curluseragent,
         serverInfoP,
         cmdline.methodName, paramArrayP,
         &resultP);
  die_if_fault_occurred(&env);
  
  dumpResult(resultP);

  Res = "undefined";
  switch (xmlrpc_value_type(resultP)) {
  case XMLRPC_TYPE_STRING:
    const char* v1;
    size_t length;
    xmlrpc_read_string_lp(&env, resultP, &length, &v1);
    Res = v1;
    break;
  case XMLRPC_TYPE_INT:
    xmlrpc_int v2;
    xmlrpc_read_int(&env, resultP, &v2);
    Res = std::to_string(v2);
    break;
  default:
    break;
  }
  
  strfree(url);
    
  xmlrpc_DECREF(resultP);
  
  freeCmdline(cmdline);
  
  xmlrpc_env_clean(&env);
  
  return 0;
}
