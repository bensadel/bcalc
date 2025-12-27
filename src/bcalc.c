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

void remove_spaces(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (!isspace((unsigned char)*src)) {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

void skip_spaces() {
    while (expr && isspace(*expr)) expr++;
}

double parse_factor() {
    skip_spaces();
    double value = 0.0;

    if (*expr == '(') {
        expr++;
        value = parse_expression();
        skip_spaces();
        if (*expr == ')') expr++;
        else printf("ERROR: missing closing parenthesis\n");
    }
    else if (*expr == 's' || *expr == 'S') { // sqrt
        expr++;
        value = sqrt(parse_factor());
    }
    else if (*expr == '-') { // unary minus
        expr++;
        value = -parse_factor();
    }
    else if (isdigit(*expr) || *expr == '.') {
        char *end;
        value = strtod(expr, &end);
        expr = end;
    }
    else if (strncasecmp(expr, "ans", 3) == 0) {
        expr += 3;
        value = ans;
    }
    else if (*expr != '\0') {
        printf("ERROR: invalid input near '%c'\n", *expr);
        expr++;
    }

    /* Handle postfix percent (e.g. "50%" -> 0.5).
       Keep it distinct from infix modulus '%' by only treating '%' as
       postfix when the following character is NOT a digit, '.', or '('.
       This preserves the existing modulus operator (e.g. "10 % 3"). */
    while (*expr == '%') {
        // if next char looks like the start of another factor, treat '%' as infix modulus instead
        if (expr[1] != '\0' && (isdigit((unsigned char)expr[1]) || expr[1] == '.' || expr[1] == '(')) {
            break;
        }
        expr++; // consume '%'
        value /= 100.0;
    }

    skip_spaces();
    return value;
}

double parse_power() {
    double base = parse_factor();
    skip_spaces();

    while (*expr == '^') {
        expr++;
        double exponent = parse_factor();
        base = pow(base, exponent);
        skip_spaces();
    }
    return base;
}

double parse_term() {
    double value = parse_power();
    skip_spaces();

    while (*expr == '*' || *expr == '/' || *expr == '%') {
        char op = *expr++;
        double right = parse_power();

        if (op == '*') value *= right;
        else if (op == '/') {
            if (right != 0) value /= right;
            else {
                printf("ERROR: division by zero\n");
                return 0;
            }
        }
        else if (op == '%') {
            if (right != 0) value = fmod(value, right);
            else {
                printf("ERROR: modulus by zero\n");
                return 0;
            }
        }

        skip_spaces();
    }
    return value;
}

double parse_expression() {
    double value = parse_term();
    skip_spaces();

    while (*expr == '+' || *expr == '-') {
        char op = *expr++;
        double right = parse_term();

        if (op == '+') value += right;
        else value -= right;

        skip_spaces();
    }
    return value;
}

void print_banner() {
    printf("*-------------------*\n");
    printf("Basic Calculator\n");
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

int main(int argc, char *argv[]) {

    char input[256];

    system(CLEAR);
    print_banner();

while (1) {
        char *line = readline("> ");
        if (!line) break;

        if (strlen(line) == 0) {
            free(line);
            continue;
        }

        add_history(line);

        expr = NULL;

        if (strcasecmp(line, "x") == 0 || strcasecmp(line, "exit") == 0 || strcasecmp(line, "quit") == 0 || strcasecmp(line, "q") == 0) {
            printf("Exiting calculator. Goodbye!\n");
            free(line);
            break;
        }
        else if (strcasecmp(line, "clear") == 0) {
            system(CLEAR);
            free(line);
            continue;
        }
        else if (strcasecmp(line, "man") == 0 || strcasecmp(line, "manual") == 0 || strcasecmp(line, "help") == 0) {
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
