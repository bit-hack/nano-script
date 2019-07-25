#
# TODO: move to a recursive scope object to keep track
#

_seed = 12345


def _random():
    global _seed
    x = _seed
    x ^= x >> 12 & 0xffffffff
    x ^= x << 25 & 0xffffffff
    x ^= x >> 27 & 0xffffffff
    _seed = x    & 0xffffffff
    return (x * 0x4F6CDD1D) >> 8


def random(maximum):
    return _random() % maximum if maximum else 0


def choose(*args):
    return args[random(len(args))]


def chance(percent):
    return random(100) < percent


def rand_range(lo, hi):
    return lo + random(hi-lo)


def rname():
    alpha_num = '_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLNMOPQRSTUVWXYZ0123456789'
    while True:
        size = rand_range(1, 8)
        out = alpha_num[random(26 * 2 + 1)]
        for i in range(0, size):
            out += alpha_num[random(len(alpha_num))]
        if out.lower() not in ['if', 'or', 'and', 'not', 'end', 'function', 'var']:
            break
    return out


class BacktrackError(RuntimeError):
    pass


class ScopeType(object):

    def __init__(self):
        self._list = []
        self._level = 0

    def clear(self):
        self._list = []

    def find(self):
        if len(self._list) == 0:
            raise BacktrackError()
        item = self._list[random(len(self._list))]
        return item[0]

    def add(self, name):
        self._list += [(name, self._level)]

    def new_name(self):
        id = rname()
        while id in self._list:
            id = rname
        return id

    def empty(self):
        return len(self._list) == 0

    def enter(self):
        self._level += 1

    def leave(self):
        self._level -= 1
        self._list = filter(lambda x: x[1] <= self._level, self._list)

    def remove(self, name):
        self._list = filter(lambda x: x[0] != name, self._list)


class Scope(object):

    def __init__(self, parent):
        self._level = 0
        self.args = ScopeType()
        self.arrays = ScopeType()
        self.vars = ScopeType()
        self.functions = ScopeType()
        self.sigs = {}
        self._parent = parent

    def new_child(self):
        return Scope(self)

    def push_to_parent(self):
        pass

    def add_func_sig(self, name, args):
        self.sigs[name] = [args]

    def get_func_sig(self, name):
        return self.sigs[name]

    def enter(self):
        self._level += 1
        self.arrays.enter()
        self.vars.enter()
        self.args.enter()

    def leave(self):
        self._level -= 1
        self.arrays.leave()
        self.vars.leave()
        self.args.leave()


class Generator(object):

    def __init__(self):
        self._indent = 0

    def indent(self):
        return ' ' * self._indent * 2 if self._indent else ''

    # program ::= ( var_decl '\n' | array_decl '\n' | function_decl '\n' )+
    def do_program(self):
        while True:
            try:
                scope = Scope(None)
                out = ''
                for _ in range(0, 1 + random(8)):
                    i = random(3)
                    if i == 0:
                        out += self.do_var_decl(scope, False) + '\n\n'
                    if i == 1:
                        out += self.do_array_decl(scope) + '\n\n'
                    if i == 2:
                        out += self.do_function_decl(scope) + '\n\n'
                return out
            except BacktrackError:
                pass

    # identifier ::= [a-fA-F_] [0-9a-fA-F_]*
    def do_identifier(self, scope):
        if scope.args.empty() and scope.vars.empty():
            raise BacktrackError()
        arg = scope.args.find()
        var = scope.vars.find()
        if arg and var:
            return choose(arg, var)
        return arg or var

    # number ::= '-'? [0-9]+
    def do_number(self, scope, base=0):
        return str(base + random(128))

    # string ::= '"' [a-zA-Z0-9]* '"'
    def do_string(self, scope):
        s = ""
        l = random(8)
        for i in range(0, l):
            s += chr( 32 + random(94) )
        return '\"' + s.replace('"', '') + '\"'

    # literal ::= number
    #           | string
    #           | none
    def do_literal(self, scope):
        while True:
            try:
                i = random(3)
                if i == 0:
                    return self.do_number(scope)
                if i == 1:
                    return self.do_string(scope)
                if i == 2:
                    return "none"

            except BacktrackError:
                pass

    # expression ::= identifier
    #              | number
    #              | function_call
    #              | '(' expression ')'
    #              | identifier '[' expression ']'
    #              | 'not' expression
    #              | expression '*' expression
    #              | expression '/' expression
    #              | expression '%' expression
    #              | expression '+' expression
    #              | expression '-' expression
    #              | expression '<' expression
    #              | expression '>' expression
    #              | expression '<=' expression
    #              | expression '>=' expression
    #              | expression '==' expression
    #              | expression 'and' expression
    #              | expression 'or' expression
    def do_expression(self, scope, complexity=3):
        while True:
            try:
                umin = '-' if chance(10) else ''
                i = random(10)
                if i == 0:
                    return umin + scope.vars.find()
                if i == 1:
                    return umin + self.do_literal(scope)
                if complexity <= 0:
                    raise BacktrackError()
                if i == 2:
                    return umin + self.do_function_call(scope, complexity-1)
                if complexity == 1:
                    raise BacktrackError()
                if i == 3:
                    return ' not ' + self.do_expression(scope, complexity)
                if i == 4:
                    return umin + '(' + self.do_expression(scope, complexity-1) + ')'
                if i == 5:
                    return umin + scope.arrays.find() + '[' + self.do_expression(scope, complexity-1) + ']'
                if i >= 6:
                    return self.do_expression(scope, complexity-1) + ' ' +\
                           choose('+', '-', '*', '/', 'and', 'or', '>', '>=', '<', '<=', '==') + ' ' +\
                           self.do_expression(scope, complexity-1)

            except BacktrackError:
                pass

    # expression_list ::= expression ( ',' expression )*
    def do_expression_list(self, scope, num_exprs, complexity=1):
        while True:
            try:
                count = num_exprs
                out = ''
                if count > 0:
                    out = self.do_expression(scope, complexity)
                for i in range(1, count):
                    out += ', ' + self.do_expression(scope, complexity)
                return out
            except BacktrackError:
                pass

    # array_decl ::= 'var' identifier '[' number ']'
    def do_array_decl(self, scope):
        name = scope.arrays.new_name()
        out = self.indent() + 'var ' + name + '[' + self.do_number(scope, 2) + ']'
        scope.arrays.add(name)
        return out

    # var_decl ::= 'var' identifier ( '=' number )?
    def do_var_decl(self, scope, can_expr):
        name = scope.vars.new_name()
        out = self.indent() + 'var ' + name
        if chance(50):
            out += ' = '
            out += self.do_expression(scope) if can_expr else self.do_literal(scope)
        scope.vars.add(name)
        return out

    # arg_decl_list ::= identifier ( ',' identifier )+
    def do_arg_decl_list(self, scope, num_args):
        out = ''
        for x in range(0, num_args):
            name = scope.args.new_name()
            out += (', ' if x else '') + name
            scope.args.add(name)
        return out

    # function_call ::= identifier '(' expression_list? ')'
    def do_function_call(self, scope, complexity=2):
        name = scope.functions.find()
        num_args = scope.get_func_sig(name)[0]

        out = ''
        out += name + '('
        out += self.do_expression_list(scope, num_args, complexity-1)
        out += ')'
        return out

    # function_decl ::= 'function' identifier '(' arg_decl_list? ')' '\n' ( statement )* 'end'
    def do_function_decl(self, scope):

        scope.args.clear()

        name = scope.functions.new_name()
        num_args = random(8)
        scope.add_func_sig(name, num_args)
        scope.functions.add(name)

        out = 'function ' + name + '('
        out += self.do_arg_decl_list(scope, num_args)
        out += ')\n'
        self._indent += 1
        scope.enter()
        for x in range(0, 8):
            out += self.do_statement(scope)
        scope.leave()
        self._indent -= 1
        out += 'end'

        return out

    # if_statement ::= 'if' '(' expression ')' '\n' ( statement )* 'end'
    def do_if_statement(self, scope):
        out = self.indent() + 'if (' + self.do_expression(scope) + ')\n'
        self._indent += 1
        scope.enter()
        for x in range(0, random(6)):
            out += self.do_statement(scope)
        scope.leave()
        self._indent -= 1
        out += self.indent() + 'end\n'
        return out

    # while_statement ::= 'while' '(' expression ')' '\n' ( statement )* 'end' '\n'
    def do_while_statement(self, scope):
        out = self.indent() + 'while (' + self.do_expression(scope) + ')\n'
        self._indent += 1
        scope.enter()
        for x in range(0, random(6)):
            out += self.do_statement(scope)
        scope.leave()
        self._indent -= 1
        out += self.indent() + 'end\n'
        return out

    # return_statement ::= 'return' + expression '\n'
    def do_return_statement(self, scope):
        return self.indent() + 'return ' + self.do_expression(scope) + '\n'

    # assign_statement ::= identifier '=' expression '\n'
    def do_assign_statement(self, scope):
        try:
            return self.indent() + self.do_identifier(scope) + ' = ' + self.do_expression(scope) + '\n'
        except BacktrackError:
            return ''

    # accumulate_statement ::= identifier '+=' expression '\n'
    def do_accum_statement(self, scope):
        try:
            return self.indent() + self.do_identifier(scope) + ' += ' + self.do_expression(scope) + '\n'
        except BacktrackError:
            return ''

    # assign_array_statement ::= identifier '[' expression ']' '=' expression '\n'
    def do_assign_array_statement(self, scope):
        try:
            return self.indent() + scope.arrays.find() + '[' + self.do_expression(scope) + ']' + ' = ' +\
                   self.do_expression(scope) + '\n'
        except BacktrackError:
            return ''

    def do_function_call_statement(self, scope):
        try:
            return self.indent() + self.do_function_call(scope) + '\n'
        except BacktrackError:
            return ''

    # statement ::= return_statement
    #             | if_statement
    #             | while_statement
    #             | function_call
    #             | assign_statement
    #             | accumulate_statement
    #             | assign_array_statement
    #             | var_decl
    #             | array_decl
    def do_statement(self, scope):
        while True:
            try:
                i = random(9)
                if i == 0:
                    return self.do_return_statement(scope)
                if i == 1:
                    return self.do_if_statement(scope)
                if i == 2:
                    return self.do_while_statement(scope)
                if i == 3:
                    return self.do_function_call_statement(scope)
                if i == 4:
                    return self.do_assign_statement(scope)
                if i == 5:
                    return self.do_accum_statement(scope)
                if i == 6:
                    return self.do_assign_array_statement(scope)
                if i == 7:
                    return self.do_var_decl(scope, True) + '\n'
                if i == 8:
                    return self.do_array_decl(scope) + '\n'
                if i >= 9:
                    assert (not "unreachable")
            except BacktrackError:
                pass


def main():
    global _seed
    base = 1024
    for i in range(0, 1024):
        _seed = 12345 + i
        gen = Generator()
        out = '# SEED {0}\n\n'.format(_seed)
        out += gen.do_program()

        with open('tests/test{0}.txt'.format(base + i), 'w') as fd:
            fd.write(out)


main()
