// version 2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <grp.h>
#include <readline/readline.h>
#include <readline/history.h>
#define CLEAR "clear"

const char *expr;
double ans = 0.0;

double parse_expression();

void remove_spaces(char *str)
{
    char *src = str, *dst = str;
    while (*src)
    {
        if (!isspace((unsigned char)*src))
        {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

void skip_spaces()
{
    while (expr && isspace(*expr))
        expr++;
}

double parse_factor()
{
    skip_spaces();
    double value = 0.0;

    if (*expr == '(')
    {
        expr++;
        value = parse_expression();
        skip_spaces();
        if (*expr == ')')
            expr++;
        else
            printf("ERROR: missing closing parenthesis\n");
    }
    else if (*expr == 's' || *expr == 'S')
    { // sqrt
        expr++;
        value = sqrt(parse_factor());
    }
    else if (*expr == '-')
    { // unary minus
        expr++;
        value = -parse_factor();
    }
    else if (isdigit(*expr) || *expr == '.')
    {
        char *end;
        value = strtod(expr, &end);
        expr = end;
    }
    else if (strncasecmp(expr, "ans", 3) == 0)
    {
        expr += 3;
        value = ans;
    }
    else if (*expr != '\0')
    {
        printf("ERROR: invalid input near '%c'\n", *expr);
        expr++;
    }

    /* Handle postfix percent (e.g. "50%" -> 0.5).
       Keep it distinct from infix modulus '%' by only treating '%' as
       postfix when the following character is NOT a digit, '.', or '('.
       This preserves the existing modulus operator (e.g. "10 % 3"). */
    while (*expr == '%')
    {
        // if next char looks like the start of another factor, treat '%' as infix modulus instead
        if (expr[1] != '\0' && (isdigit((unsigned char)expr[1]) || expr[1] == '.' || expr[1] == '('))
        {
            break;
        }
        expr++; // consume '%'
        value /= 100.0;
    }

    skip_spaces();
    return value;
}

double parse_power()
{
    double base = parse_factor();
    skip_spaces();

    while (*expr == '^')
    {
        expr++;
        double exponent = parse_factor();
        base = pow(base, exponent);
        skip_spaces();
    }
    return base;
}

double parse_term()
{
    double value = parse_power();
    skip_spaces();

    while (*expr == '*' || *expr == '/' || *expr == '%')
    {
        char op = *expr++;
        double right = parse_power();

        if (op == '*')
            value *= right;
        else if (op == '/')
        {
            if (right != 0)
                value /= right;
            else
            {
                printf("ERROR: division by zero\n");
                return 0;
            }
        }
        else if (op == '%')
        {
            if (right != 0)
                value = fmod(value, right);
            else
            {
                printf("ERROR: modulus by zero\n");
                return 0;
            }
        }

        skip_spaces();
    }
    return value;
}

double parse_expression()
{
    double value = parse_term();
    skip_spaces();

    while (*expr == '+' || *expr == '-')
    {
        char op = *expr++;
        double right = parse_term();

        if (op == '+')
            value += right;
        else
            value -= right;

        skip_spaces();
    }
    return value;
}

void print_banner()
{
    printf("*-------------------*\n");
    printf("bcalc - the insanely simple cli calculator\n");
    printf("-------------------\n");
    printf("Directions: enter an equation such as below and click enter\n");
    printf("(1 + 1 * (s(1000/10)))^2 - (121%%10) will equal 120.00\n");
    printf("-------------------\n");
    printf("Supports: addition(+), subtraction(-), multiplication(*), division(/), modulus(%%), power(^), parentheses(), sqrt(s), previous answer(ans)\n");
    printf("Commands:\n");
    printf("clear (to clear terminal)\n");
    printf("x, exit, quit, q (quit calculator)\n");
    printf("man or manual or help (to display this menu)\n");
    printf("*-------------------*\n");
}

static int resolve_executable_path(const char *argv0, char *out, size_t outsz)
{
    if (!argv0 || !out)
        return -1;

    // If argv0 contains a slash, treat it as a path (relative or absolute)
    if (strchr(argv0, '/'))
    {
        if (realpath(argv0, out) == NULL)
            return -1;
        return 0;
    }

    // otherwise search PATH for an executable named argv0
    const char *pathenv = getenv("PATH");
    if (!pathenv)
        return -1;

    char *paths = strdup(pathenv);
    if (!paths)
        return -1;

    char *saveptr = NULL;
    char *dir = strtok_r(paths, ":", &saveptr);
    while (dir)
    {
        char candidate[PATH_MAX];
        snprintf(candidate, sizeof(candidate), "%s/%s", dir, argv0);
        if (access(candidate, X_OK) == 0)
        {
            if (realpath(candidate, out) != NULL)
            {
                free(paths);
                return 0;
            }
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(paths);
    return -1;
}

int main(int argc, char *argv[])
{
    char exe_path[PATH_MAX];
    int in_usr_local = 0;

    if (resolve_executable_path(argv[0], exe_path, sizeof(exe_path)) == 0)
    {
        if (strstr(exe_path, "/Cellar/") != NULL)
            in_usr_local = 1;
    }
    else
    {
        // fallback: couldn't resolve -> assume not in /usr/local/bin
        in_usr_local = 0;
    }

    if (argc > 1)
    {
        // Catch explicit version flags
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)
        {
            char *cellar = strstr(exe_path, "/Cellar/bcalc/");
            if (cellar)
            {
                cellar += strlen("/Cellar/bcalc/");
                char version[PATH_MAX];
                int i = 0;
                while (cellar[i] && cellar[i] != '/' && i < PATH_MAX - 1)
                {
                    version[i] = cellar[i];
                    i++;
                }
                version[i] = '\0';
                printf("%s\n", version);
                return 0;
            }
        }

        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        {
            if (in_usr_local)
            {
                printf("*-------------------*\n");
                printf("bcalc - the insanely simple cli calculator\n");
                printf("-------------------\n");
                printf("Enter 'bcalc' in your terminal to invoke the program\n");
                printf("-------------------\n");
                printf("Enter 'man bcalc' in your terminal to see the program manual\n");
                return 0;
            }
        }

        // If installed, reject any other args (only allow bare program or -v/--version)
        if (in_usr_local)
        {
            fprintf(stderr, "ERROR: Invalid flag. See manual for valid arguements\n");
            return 1;
        }
    }

    char input[256];

    system(CLEAR);
    print_banner();

    while (1)
    {
        char *line = readline("> ");
        if (!line)
            break;

        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        add_history(line);

        expr = NULL;

        if (strcasecmp(line, "x") == 0 || strcasecmp(line, "exit") == 0 || strcasecmp(line, "quit") == 0 || strcasecmp(line, "q") == 0)
        {
            printf("Exiting calculator. Goodbye!\n");
            free(line);
            break;
        }
        else if (strcasecmp(line, "clear") == 0)
        {
            system(CLEAR);
            free(line);
            continue;
        }
        else if (strcasecmp(line, "man") == 0 || strcasecmp(line, "manual") == 0 || strcasecmp(line, "help") == 0)
        {
            print_banner();
            free(line);
            continue;
        }

        remove_spaces(line);

        expr = line;
        double result = parse_expression();

        if (*expr != '\0')
            printf("WARNING: unexpected input at '%s'\n", expr);

        printf("= %.2lf\n", result);
        ans = result;

        free(line);
    }

    return 0;
}
