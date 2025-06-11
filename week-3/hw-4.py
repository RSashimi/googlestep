def read_number(line, index):
    number = 0
    while index < len(line) and line[index].isdigit():
        number = number * 10 + int(line[index])
        index += 1
    if index < len(line) and line[index] == '.':
        index += 1
        decimal = 0.1
        while index < len(line) and line[index].isdigit():
            number += int(line[index]) * decimal
            decimal /= 10
            index += 1
    token = {'type': 'NUMBER', 'number': number}
    return token, index
""" read_number function:
Reads a number from the input string and returns it as a token
Handles both integer and decimal parts
Returns the number token and the next index position """

def read_plus(line, index):
    token = {'type': 'PLUS'}
    return token, index + 1

def read_minus(line, index):
    token = {'type': 'MINUS'}
    return token, index + 1

def read_multiply(line, index):
    token = {'type': 'MULTIPLY'}
    return token, index + 1

def read_divide(line, index):
    token = {'type': 'DIVIDE'}
    return token, index + 1

def read_left_paren(line, index):
    token = {'type': 'LEFT_PAREN'}
    return token, index + 1

def read_right_paren(line, index):
    token = {'type': 'RIGHT_PAREN'}
    return token, index + 1

def read_abs(line, index):
    token = {'type': 'ABS'}
    return token, index + 1

def read_round(line, index):
    token = {'type': 'ROUND'}
    return token, index + 1

def read_int(line, index):
    token = {'type': 'INT'}
    return token, index + 1
"""Operator reading functions:
Each function reads and tokenizes a specific operator (+,-,*,/,(),)
Simple implementation that creates appropriate token for each operator type"""


def tokenize(line):
    tokens = []
    index = 0
    while index < len(line):
        if line[index].isdigit():
            (token, index) = read_number(line, index)
        elif line[index] == '+':
            (token, index) = read_plus(line, index)
        elif line[index] == '-':
            (token, index) = read_minus(line, index)
        elif line[index] == '*':
            (token, index) = read_multiply(line, index)
        elif line[index] == '/':
            (token, index) = read_divide(line, index)
        elif line[index] == '(':
            (token, index) = read_left_paren(line, index)
        elif line[index] == ')':
            (token, index) = read_right_paren(line, index)
        elif line[index] == 'abs()':
            (token, index) = read_abs(line, index)
        elif line[index] == 'round()':
            (token, index) = read_round(line, index)
        elif line[index] == 'int()':
            (token, index) = read_int(line, index)
        elif line[index].isspace():
            index += 1
            continue
        else:
            print('Invalid character found: ' + line[index])
            exit(1)
        tokens.append(token)
    return tokens
"""tokenize function:
Parses input string into a sequence of tokens
Identifies numbers, operators and parentheses
Skips whitespace and exits on invalid characters"""


def evaluate_multiply_divide(tokens):
    new_tokens = []
    index = 0

    while index < len(tokens):
        token = tokens[index]
        if token['type'] == 'NUMBER':
            if index > 0 and tokens[index - 1]['type'] in ('MULTIPLY', 'DIVIDE'):
                op = tokens[index - 1]['type']
                prev_token = new_tokens.pop()
                if op == 'MULTIPLY':
                    number = prev_token['number'] * token['number']
                else:
                    number = prev_token['number'] / token['number']
                new_tokens.append({'type': 'NUMBER', 'number': number})
            else:
                new_tokens.append(token)
        else:
            new_tokens.append(token)
        index += 1
    return new_tokens
"""evaluate_multiply_divide function:
Handles multiplication and division operations with priority
Processes tokens according to operator precedence
Returns new token list with multiplications and divisions resolved"""


def evaluate_plus_minus(tokens):
    new_tokens = []
    new_tokens.insert(0, {'type': 'PLUS'})  
    answer = 0
    index = 1
    while index < len(tokens):
        if tokens[index]['type'] == 'NUMBER':
            if tokens[index - 1]['type'] == 'PLUS':
                answer += tokens[index]['number']
            elif tokens[index - 1]['type'] == 'MINUS':
                answer -= tokens[index]['number']
            else:
                print("Invalid syntax !!!!")
                exit(1)
        index += 1
    return answer
"""evaluate_plus_minus function:
Processes addition and subtraction operations
Inserts leading PLUS for uniform processing
Returns final calculation result"""


def evaluate_parentheses(tokens):
    stack = []
    output = []
    
    for token in tokens:
        if token['type'] == 'LEFT_PAREN':
            stack.append(token)
        elif token['type'] == 'RIGHT_PAREN':
            while stack and stack[-1]['type'] != 'LEFT_PAREN':
                output.append(stack.pop())
            if stack:  # Remove left parenthesis
                stack.pop()
        else:
            output.append(token)

            
    return output
"""evaluate_parentheses function:
Handles expressions within parentheses
Uses stack to manage nested parentheses
Returns tokens with parentheses resolved"""

def evaluate_abs(tokens):
    new_tokens = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token['type'] == 'ABS':
            if index + 1 < len(tokens) and tokens[index + 1]['type'] == 'NUMBER':
                number = abs(tokens[index + 1]['number'])
                new_tokens.append({'type': 'NUMBER', 'number': number})
                index += 2
            else:
                print("Invalid abs() syntax")
                exit(1)
        else:
            new_tokens.append(token)
            index += 1
    return new_tokens

def evaluate_round(tokens):
    new_tokens = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token['type'] == 'ROUND':
            if index + 1 < len(tokens) and tokens[index + 1]['type'] == 'NUMBER':
                number = round(tokens[index + 1]['number'])
                new_tokens.append({'type': 'NUMBER', 'number': number})
                index += 2
            else:
                print("Invalid round() syntax")
                exit(1)
        else:
            new_tokens.append(token)
            index += 1
    return new_tokens

def evaluate_int(tokens):
    new_tokens = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token['type'] == 'INT':
            if index + 1 < len(tokens) and tokens[index + 1]['type'] == 'NUMBER':
                number = int(tokens[index + 1]['number'])
                new_tokens.append({'type': 'NUMBER', 'number': number})
                index += 2
            else:
                print("Invalid int() syntax")
                exit(1)
        else:
            new_tokens.append(token)
            index += 1
    return new_tokens


def evaluate(tokens):
    # First handle parentheses
    tokens = evaluate_parentheses(tokens)
    # Then handle multiply and divide
    tokens = evaluate_multiply_divide(tokens)
    # Finally handle plus and minus
    return evaluate_plus_minus(tokens)
"""evaluate function:
Controls overall calculation flow
Executes three-stage calculation following operator precedence
Returns final calculation result"""


def test(line):
    tokens = tokenize(line)
    actual_answer = evaluate(tokens)
    expected_answer = eval(line)
    if abs(actual_answer - expected_answer) < 1e-8:
        print("PASS! (%s = %f)" % (line, expected_answer))
    else:
        print("FAIL! (%s should be %f but was %f)" % (line, expected_answer, actual_answer))
"""test function:
Verifies calculation accuracy
Compares results with Python's built-in eval()
Uses floating-point error tolerance in comparisons"""


def run_test():
    print("==== Test started! ====")

    # normal calculation
    test("1")
    test("1+2")
    test("1-2")
    test("2*3")
    test("8/2")

    # calculation of decimals
    test("1.0+2")
    test("1.0+2.0")
    test("2.5-0.5")
    test("3.0*1.5")
    test("7.2/3.6")

    # mixed calculations
    test("6+4/2")       
    test("1+2*3")       
    test("10-6/3")     
    test("8/2+3")       

    # parentheses calculations
    test("(1+2)*3")
    test("2*(3+4)")
    test("(2+3)*(4+5)")
    test("((1+2)*3)+4")

    # mixed mixed mixed calculations
    test("1+2+3+4")
    test("10-2-3")
    test("2*3*4")
    test("8/2/2")
    test("1+2*3-4/2")  
    test("(1+2)*(3-4)/2")

    print("==== Test finished! ====\n")
"""run_test function:
Executes comprehensive test cases
Tests basic calculations, decimal operations, and parentheses
Includes edge cases for thorough verification"""
    
run_test()


while True:
    print('> ', end="")
    line = input()
    tokens = tokenize(line)
    answer = evaluate(tokens)
    print("answer = %f\n" % answer)
"""Main loop:
Provides interactive calculator functionality
Accepts user input and displays results
Enables continuous calculation processing"""
