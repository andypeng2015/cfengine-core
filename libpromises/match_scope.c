/*
  Copyright 2023 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; version 3.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <match_scope.h>

#include <matching.h>
#include <eval_context.h>
#include <string_lib.h>                                   /* StringFromLong */
#include <regex.h>                                        /* CompileRegex */


/* Sets variables */
static bool RegExMatchSubString(EvalContext *ctx, Regex *regex, const char *teststring, int *start, int *end)
{
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(regex, NULL);
    int result = pcre2_match(regex, (PCRE2_SPTR) teststring, PCRE2_ZERO_TERMINATED,
                             0, 0, match_data, NULL);
    /* pcre2_match() returns the highest capture group number + 1, i.e. 1 means
     * a match with 0 capture groups. 0 means the vector of offsets is small,
     * negative numbers are errors (incl. no match). */
    if (result > 0)
    {
        size_t *ovector = pcre2_get_ovector_pointer(match_data);
        *start = ovector[0];
        *end = ovector[1];

        EvalContextVariableClearMatch(ctx);

        for (int i = 0; i < result; i++)        /* make backref vars $(1),$(2) etc */
        {
            const char *backref_start = teststring + ovector[i * 2];
            int backref_len = ovector[i * 2 + 1] - ovector[i * 2];

            if (backref_len < CF_MAXVARSIZE)
            {
                char substring[CF_MAXVARSIZE];
                char *index = StringFromLong(i);
                strlcpy(substring, backref_start, MIN(CF_MAXVARSIZE, backref_len + 1));
                EvalContextVariablePutSpecial(ctx, SPECIAL_SCOPE_MATCH, index, substring, CF_DATA_TYPE_STRING, "source=regex");
                free(index);
            }
        }
    }
    else
    {
        *start = 0;
        *end = 0;
    }

    pcre2_match_data_free(match_data);
    RegexDestroy(regex);
    return result > 0;
}

/* Sets variables */
static bool RegExMatchFullString(EvalContext *ctx, Regex *rx, const char *teststring)
{
    int match_start;
    int match_len;

    if (RegExMatchSubString(ctx, rx, teststring, &match_start, &match_len))
    {
        return ((size_t) match_start == 0) && ((size_t) match_len == strlen(teststring));
    }
    else
    {
        return false;
    }
}

bool FullTextMatch(EvalContext *ctx, const char *regexp, const char *teststring)
{
    if (strcmp(regexp, teststring) == 0)
    {
        return true;
    }

    Regex *rx = CompileRegex(regexp);
    if (rx == NULL)
    {
        return false;
    }

    if (RegExMatchFullString(ctx, rx, teststring))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ValidateRegEx(const char *regex)
{
    Regex *rx = CompileRegex(regex);
    bool regex_valid = rx != NULL;

    RegexDestroy(rx);
    return regex_valid;
}

bool BlockTextMatch(EvalContext *ctx, const char *regexp, const char *teststring, int *start, int *end)
{
    Regex *rx = CompileRegex(regexp);

    if (rx == NULL)
    {
        return false;
    }

    if (RegExMatchSubString(ctx, rx, teststring, start, end))
    {
        return true;
    }
    else
    {
        return false;
    }
}
