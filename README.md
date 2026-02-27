# Отчёт к практическому заданию №1

**Дисциплина:** Языки программирования
**Студент:** Богданов Александр
**Группа:** Р4119
**Факультет:** ПИиКТ, Университет ИТМО
**Год:** 2025/2026

---

## 1. Цель

Реализовать модуль синтаксического анализа текста на языке варианта 4, выполняющий построение синтаксического дерева (AST) по исходному файлу, и вывести полученное дерево в формате, допускающем графическое представление.

## 2. Задачи

1. Изучить и выбрать средство синтаксического анализа - реализован парсер вручную методом рекурсивного спуска (recursive descent parser), управляемый грамматикой варианта 4.
2. Записать спецификацию синтаксической структуры разбираемого языка, включающую функции с аргументами и возвращаемым типом, ветвления `if`-`else`, циклы `while`/`until`/`repeat`, литералы, выражения арифметики и логики, массивы, вызовы функций.
3. Реализовать модуль (лексер + парсер), принимающий строку с текстом и возвращающий дерево разбора и коллекцию ошибок.
4. Реализовать тестовую программу, принимающую входной и выходной файл через аргументы командной строки, выводящую ошибки в stderr.
5. Подготовить тесты, покрывающие все синтаксические конструкции.

## 3. Описание работы

### Состав модулей

| Модуль             | Файлы                    | Назначение                                |
| ------------------ | ------------------------ | ----------------------------------------- |
| Лексер             | `lexer.h`, `lexer.cpp`   | Разбиение исходного текста на токены      |
| Парсер             | `parser.h`, `parser.cpp` | Построение AST из потока токенов          |
| AST                | `ast.h`                  | Структуры данных дерева разбора           |
| DOT-экспорт        | `dot_export.h`, `.cpp`   | Сериализация дерева в формат Graphviz DOT |
| JSON-экспорт       | `json_export.h`, `.cpp`  | Сериализация дерева в формат JSON         |
| Тестовая программа | `main.cpp`               | CLI-обёртка, чтение файлов, вывод ошибок  |

**Размер реализации:** 1122 строки кода (лексер 235, парсер 616, экспорт 133, main 138).

### Программный интерфейс модуля

```cpp
// Лексический анализ
Lexer lexer(sourceText);
std::vector<Token> tokens = lexer.tokenize();
const std::vector<LexerError>& errors = lexer.errors();

// Синтаксический анализ
Parser parser(tokens);
ParseResult result = parser.parse();
// result.tree - корень AST (или nullptr)
// result.errors - коллекция ParseError с позициями
```

### Использование тестовой программы

```bash
# Формат DOT (по умолчанию)
./build/parser test/example.v4 test/example.dot

# Формат JSON
./build/parser --format=json test/example.v4 test/example.json
```

Ошибки выводятся в stderr в формате `файл:строка:колонка: тип ошибки: сообщение`.

### Структуры данных результата разбора

Дерево разбора представлено структурой `ASTNode`:

```cpp
struct ASTNode {
    enum Kind {
        // Верхний уровень
        SOURCE,            // корень - список определений
        FUNC_DEF,          // определение функции
        FUNC_SIGNATURE,    // сигнатура: имя, аргументы, тип возврата
        FUNC_ARG,          // аргумент функции

        // Типы
        TYPE_BUILTIN,      // встроенный тип (int, bool, string, ...)
        TYPE_CUSTOM,       // пользовательский тип (идентификатор)
        TYPE_ARRAY,        // массивный тип (typeRef array[dim])

        // Инструкции
        STMT_IF,           // if-then[-else]
        STMT_LOOP,         // while/until ... end
        STMT_REPEAT,       // statement while/until expr ;
        STMT_BREAK,        // break ;
        STMT_EXPR,         // expr ;
        STMT_BLOCK,        // begin...end или {...}
        STMT_ASSIGN,       // expr = expr ;

        // Выражения
        EXPR_BINARY,       // expr op expr
        EXPR_UNARY,        // op expr
        EXPR_BRACES,       // ( expr )
        EXPR_CALL,         // expr ( args )
        EXPR_SLICE,        // expr [ ranges ]
        EXPR_RANGE,        // expr .. expr
        EXPR_PLACE,        // идентификатор
        EXPR_LITERAL,      // числовой, строковый, логический литерал
    };

    Kind kind;                        // тип узла
    SourceLocation loc;               // позиция в исходном тексте (строка, столбец)
    std::string value;                // текст токена (имя, оператор, значение литерала)
    std::vector<ASTNodePtr> children; // дочерние узлы
};
```

Каждый узел хранит свой тип (`Kind`), позицию в исходном тексте, опциональное значение и список дочерних узлов. Дерево владеет потомками через `std::unique_ptr<ASTNode>`, что гарантирует автоматическое освобождение памяти.

## 4. Аспекты реализации

### Лексер

Лексер реализован как однопроходный сканер. Метод `tokenize()` последовательно читает символы, распознаёт токены и формирует массив `Token`. Поддерживает все типы литералов: десятичные (`42`), шестнадцатеричные (`0xFF`), двоичные (`0b10110`), строки (`"hello"`), символы (`'A'`), булевы (`true`/`false`). Комментарии (`// ...`) пропускаются.

Ключевые слова распознаются через таблицу `keywordType()` после чтения идентификатора.

### Парсер - рекурсивный спуск

Парсер реализован вручную методом рекурсивного спуска без генераторов. Каждому нетерминалу грамматики варианта 4 соответствует метод:

```
parseSource          → sourceItem*
parseFuncDef         → 'def' funcSignature statement* 'end'
parseStatement       → if | loop | break | block | exprOrAssign
parseIfStatement     → 'if' expr 'then' stmt ('else' stmt)?
parseLoopStatement   → ('while'|'until') expr stmt* 'end'
parseBlock           → ('begin'|'{') (stmt|sourceItem)* ('end'|'}')
parseExpressionOrAssign → expr ('=' expr)? (('while'|'until') expr)? ';'
```

### Дополнительная обработка: приоритет операторов

На уровне грамматики выражения описаны как `expr binOp expr` - линейная форма без приоритета. Для построения иерархического дерева выражений, корректно отражающего приоритеты операторов, в парсере реализована цепочка методов по методу рекурсивного спуска с приоритетами (precedence climbing):

```
parseExpression → parseExprOr
parseExprOr → parseExprAnd ('||' parseExprAnd)*
parseExprAnd → parseExprComparison ('&&' parseExprComparison)*
parseExprComparison → parseExprBitOr (('<'|'>'|...) parseExprBitOr)*
parseExprBitOr → parseExprBitXor ('|' parseExprBitXor)*
parseExprBitXor → parseExprBitAnd ('^' parseExprBitAnd)*
parseExprBitAnd → parseExprShift ('&' parseExprShift)*
parseExprShift → parseExprAdd (('<<'|'>>') parseExprAdd)*
parseExprAdd → parseExprMul (('+'|'-') parseExprMul)*
parseExprMul → parseExprUnary (('*'|'/'|'%') parseExprUnary)*
parseExprUnary → ('-'|'~'|'!'|'++'|'--')? parseExprPostfix
parseExprPostfix → parseExprPrimary (call | slice | '++'|'--')*
```

Таким образом, выражение `a + b * c` строится как `BinaryExpr(+, a, BinaryExpr(*, b, c))`, а не линейно.

### Обработка ошибок

При встрече непредвиденного токена парсер сохраняет ошибку в коллекцию `ParseError` с указанием позиции и выполняет синхронизацию - пропуск токенов до следующей опорной точки (`;`, `end`, `def`), после чего продолжает разбор. Это позволяет обнаружить несколько ошибок за один проход.

```cpp
void Parser::synchronize() {
    while (!isAtEnd()) {
        if (check(TOK_SEMICOLON)) { advance(); return; }
        if (check(TOK_END) || check(TOK_DEF)) return;
        advance();
    }
}
```

## 5. Результаты тестирования

### Пример 1: Функции, аргументы, типы

Вход (`example.v4`):

```
def add(a of int, b of int) of int
    a + b;
end
```

Результат (DOT):

```
FuncDef
  FuncSignature "add"
      FuncArg "a"
          TypeBuiltin "int"
      FuncArg "b"
          TypeBuiltin "int"
        TypeBuiltin "int"   
    ExprStmt
      BinaryExpr "+"
        Place "a"
        Place "b"
```

### Пример 2: Присваивание и литералы всех видов

Вход:

```
def main()
    x = 42;
    y = 0xFF;
    z = 0b10110;
    name = "hello world";
    ch = 'A';
    flag = true;
end
```

Результат:

```
FuncDef
  FuncSignature "main"
    Assign
      Place "x"
      Literal "42"        (десятичный)
    Assign
      Place "y"
      Literal "0xFF"      (шестнадцатеричный)
    Assign
      Place "z"
      Literal "0b10110"   (двоичный)
    Assign
      Place "name"
      Literal "hello world" (строка)
    Assign
      Place "ch"
      Literal "A"         (символ)
    Assign
      Place "flag"
      Literal "true"      (булевский)
```

### Пример 3: If-then-else

Вход:

```
if flag then
    x = 1;
else
    x = 0;
```

Результат:

```
If
  Place "flag"           
    Assign                
      Place "x"
      Literal "1"
    Assign                
      Place "x"
      Literal "0"
```

### Пример 4: Циклы (while, until, repeat)

Вход:

```
while x > 0
    x = x - 1;
end

until x == 10
    x = x + 1;
end

x = x + 1 while x < 10;
x = x - 1 until x == 0;
```

Результат:

```
Loop "while"
  BinaryExpr ">"
    Place "x"
    Literal "0"
  Assign
    Place "x"
    BinaryExpr "-" [x, 1]

Loop "until"
  BinaryExpr "==" [x, 10]
  Assign ...

Repeat "while"
  Assign [x = x + 1]        
  BinaryExpr "<" [x, 10]    

Repeat "until"
  Assign [x = x - 1]
  BinaryExpr "==" [x, 0]
```

### Пример 5: Break, блоки

Вход:

```
while true
    if x == 5 then break;
    x = x + 1;
end

begin
    x = 1;
    y = 2;
end
```

Результат:

```
Loop "while"
  Literal "true"
  If
      BinaryExpr "==" [x, 5]
      Break
  Assign [x = x + 1]

Block
  Assign [x = 1]
  Assign [y = 2]
```

### Пример 6: Обработка ошибок

Вход (`bad.v4`):

```
def foo() 123 @@@ end
```

Вывод в stderr:

```
bad.v4:1:15: lexer error: Unexpected character: '@'
bad.v4:1:16: lexer error: Unexpected character: '@'
bad.v4:1:17: lexer error: Unexpected character: '@'
```

Дерево частично построено (123 распознан как литерал, ошибочные токены помечены).

## 6. Результаты

1. Реализован лексер (235 строк), распознающий все типы токенов варианта 4: литералы, ключевые слова, операторы, разделители.
2. Реализован парсер методом рекурсивного спуска (616 строк), строящий иерархическое AST со всеми конструкциями грамматики.
3. Выражения преобразуются из линейной формы грамматики в иерархическое дерево с корректными приоритетами операторов через цепочку из 11 уровней разбора.
4. Реализованы два формата вывода: Graphviz DOT (для визуализации) и JSON (для программной обработки).
5. Тестовая программа принимает входной/выходной файл через CLI, ошибки выводятся в stderr.
6. Парсер обнаруживает несколько ошибок за один проход благодаря механизму синхронизации.
7. Протестированы все синтаксические конструкции варианта 4 (11 примеров).

## 7. Выводы

Цель задания достигнута: создан модуль синтаксического анализа для языка варианта 4, формирующий полное иерархическое синтаксическое дерево.

Парсер, реализованный вручную методом рекурсивного спуска, оказался достаточен для грамматики варианта 4 - она не содержит левой рекурсии и допускает LL(1)-разбор с просмотром на один токен вперёд. Основная сложность заключалась в преобразовании выражений из плоского бинарного формата `expr binOp expr` в дерево с приоритетами - для этого была реализована цепочка из 11 уровней разбора, соответствующих приоритетам операторов языка.

Механизм восстановления после ошибок (synchronize) позволяет выводить все ошибки за один проход, а не останавливаться на первой. Все ресурсы управляются через `unique_ptr`, утечки памяти отсутствуют.

Результат работы модуля - дерево `ASTNode` - используется в последующих заданиях (построение CFG, кодогенерация) без изменений интерфейса.
