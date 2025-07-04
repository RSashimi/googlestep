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
        else:
            print('Invalid character found: ' + line[index])
            exit(1)
        tokens.append(token)
    return tokens

# fix!

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
        elif token['type'] in ('PLUS', 'MINUS'):
            new_tokens.append(token)
        index += 1
    return new_tokens

def evaluate_plus_minus(tokens):
    new_tokens = []
    tokens.insert(0, {'type': 'PLUS'})  
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


def evaluate(tokens):
    tokens_after_multiply_divide = evaluate_multiply_divide(tokens)
    final_answer = evaluate_plus_minus(tokens_after_multiply_divide)
    return final_answer

# fix! 

def test(line):
    tokens = tokenize(line)
    actual_answer = evaluate(tokens)
    expected_answer = eval(line)
    if abs(actual_answer - expected_answer) < 1e-8:
        print("PASS! (%s = %f)" % (line, expected_answer))
    else:
        print("FAIL! (%s should be %f but was %f)" % (line, expected_answer, actual_answer))


# Add more tests to this function :)
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

    # mixed calulations
    test("6+4/2")       
    test("1+2*3")       
    test("10-6/3")     
    test("8/2+3")       

    # mixed mixed mixed calculations
    test("1+2+3+4")
    test("10-2-3")
    test("2*3*4")
    test("8/2/2")
    test("1+2*3-4/2")  

    # when its a minus
    test("2-5.5")       

    print("==== Test finished! ====\n")
    
run_test()


while True:
    print('> ', end="")
    line = input()
    tokens = tokenize(line)
    answer = evaluate(tokens)
    print("answer = %f\n" % answer)
