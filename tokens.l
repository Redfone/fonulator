/* Scanner */

%{

 #include "fonulator.h"

int yylval;

%}


%option caseless
%option nounput
%option noyywrap
%option yylineno

%s KEY_ENTRY

DIGIT		[0-9]
HEXALPHA        [a-fA-F]
HEXTAIL         ({DIGIT}|{HEXALPHA}){1,128}
HEXNUM          0[xX]{HEXTAIL}
ID		[a-z][a-z0-9]*
MAC		([a-fA-F0-9][a-fA-F0-9][:-]){5}[a-fA-F0-9][a-fA-F0-9]
IP              [0-9]{1,3}"."[0-9]{1,3}"."[0-9]{1,3}"."[0-9]{1,3}
%%

^#.*                    { /* Do nothing on comment */ } 

^"["globals"]"$         return TOK_GLOBALS;

^"["span({DIGIT})"]"$	{ 
			   yylval = atoi(&yytext[5]);
			   return TOK_SPAN;
			}

^priorities={DIGIT},{DIGIT},{DIGIT},{DIGIT} { yytext += 11; return TOK_PRIO; }

^wplloff$		return TOK_WPLLOFF;

^port=((1)|(2))$        {
                          yylval = atoi(&yytext[5]);
                          return TOK_PORT;
                        }

^fb"="[a-zA-Z0-9.]+$    {
                          yytext += 3;
                          return TOK_FB_CONFIG;
			}

^iec$                   { /* Do nothing, deprecated */ }

^framing"="((esf)|(sf)|(ccs)|(cas))$    {
                                   yytext += 8;
                                   return TOK_FRAMING;
                                }

^encoding"="((b8zs)|(hdb3)|(ami))$  {
                                      yytext += 9;
                                      return TOK_ENCODING;
                                    }

^server"="{MAC}$	{
                          yytext += 7;
			  return TOK_SERVER_CONFIG;
			}

^dsp"="((ulaw)|(alaw)|(off))     {
                                  yytext += 4;
                                  if (!strcmp(yytext, "ulaw"))
				    return TOK_ULAW;
				  else if (!strcmp(yytext, "alaw"))
				    return TOK_ALAW;
				  else if (!strcmp(yytext, "off"))
				    return TOK_DSP_OFF;
                               }

^{MAC}$                 return TOK_MAC;
^{IP}$                  return TOK_IP;

^J1$                    return TOK_J1;

^crc4$                  return TOK_CRCMF;

^rbs$			return TOK_RBS;

^loopback$              return TOK_LOOPBACK;

^slave$                 return TOK_SLAVE;
^master$                { /* Do nothing for master, it is default */ }

^longhaul=[0-3]$        { yylval = atoi(yytext+9); return TOK_LONGHAUL; }

^shorthaul=[0-4]$       { yylval = atoi(yytext+10); return TOK_SHORTHAUL; }

^dejitter$              return TOK_DEJITTER;

^"["dsp"]"$             return TOK_DSP_HEADER;
^voiceA"="              return TOK_VOICEA;
^voiceB"="              return TOK_VOICEB;
^data"="                return TOK_DATA;
^dspdisabled$           return TOK_DSP_DISABLED;
{DIGIT}+-{DIGIT}+$      return TOK_RANGE;
{DIGIT}+$               { yylval = atoi(yytext); return TOK_DIGIT; }
key_entry\ \{                                 { BEGIN(KEY_ENTRY); return TOK_BEGIN_KEY; }
<KEY_ENTRY>.*SLOT_ID\ =\ {HEXNUM};            {
                                                  char *ptr;
                                                  ptr = strstr(yytext, "0x");
                                                  if (ptr != NULL)
                                                    yytext = ptr;

                                                  BEGIN(KEY_ENTRY);
                                                  return TOK_SLOT_ID;
                                              }
<KEY_ENTRY>.*CUSTOMER_KEY\ =\ {HEXNUM};       {
                                                  char *ptr;
                                                  ptr = strstr(yytext, "0x");
                                                  if (ptr != NULL)
                                                    yytext = ptr;

                                                  BEGIN(KEY_ENTRY);
                                                  return TOK_CUSTOMER_KEY;
                                              }
<KEY_ENTRY>\}                                 { BEGIN(INITIAL);  return TOK_END_KEY; }

[ \t\n]+                                      {/* white space */}
.		                              return TOK_UNKNOWN;

%%
