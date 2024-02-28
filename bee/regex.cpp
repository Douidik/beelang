#include "regex.hpp"

namespace bee::regex
{

u64 State::submit(string expr, u64 n) const
{
    if (option != Regex_Eps and n >= expr.len)
        return npos;

    switch (option) {
    case Regex_Eps:
        return n;

    case Regex_Any:
        return n + 1;

    case Regex_None:
        return npos;

    case Regex_Not:
        return sequence->submit(expr, n) != npos ? npos : n + 1;

    case Regex_Dash:
        return sequence->submit(expr, n) != npos ? n : npos;

    case Regex_Str: {
        string substr = expr.substr(n, str.len);
        return substr.match(str) ? n + str.len : npos;
    }

    case Regex_Set:
        return str.index(expr[n]) != npos ? n + 1 : npos;

    case Regex_Scope:
        return range[0] <= expr[n] and expr[n] <= range[1] ? n + 1 : npos;

    default:
        return npos;
    }
}

Node_Set *node_set_insert(Node_Set *set, Node *node)
{
    if (!set) {
        set = new Node_Set{node, NULL};
    } else {
        auto it = set;
        auto at = it;

        while (it != NULL and it->node->id < node->id) {
            if (it->node == node) // Do not insert duplicates
                return it;

            at = it;
            it = it->next;
        }

        auto insertion = new Node_Set{node, at->next};
        at->next = insertion;
    }
    return set;
}

// todo! do not use recursion
// cannot iterate because the '.next' depends on free value
void node_set_deinit(Node_Set *set)
{
    if (set != NULL) {
        node_set_deinit(set->next);
        delete set;
    }
}

void Node::deinit()
{
    node_set_deinit(edges);
    node_set_deinit(member_cache);
}

u64 Node::submit(string expr, u64 n) const
{
    u64 match = state.submit(expr, n);

    if (match != npos) {
        if (!has_edges() and match >= expr.len)
            return match;

        for (auto it = edges; it != NULL; it = it->next) {
            u64 match_fwd = it->node->submit(expr, match);
            if (match_fwd != npos)
                return match_fwd;
        }

        if (!has_edges())
            return match;
    }

    return npos;
}

Node *Node::push(Node *node)
{
    node->map_sequence_ids(end()->id + 1);
    edges = node_set_insert(edges, node);
    on_edges_insertion();
    return node;
}

Node *Node::merge(Node *node)
{
    node->map_sequence_ids(end()->id + 1);
    return concat(node);
}

Node *Node::concat(Node *node)
{
    auto members = make_members();

    for (auto it = members; it != NULL; it = it->next) {
        if (!it->node->has_edges())
            edges = node_set_insert(edges, node);
    }

    on_edges_insertion();
    return node;
}

Node *Node::end()
{
    auto members = make_members();
    Node *node_end = this;

    for (auto it = members; it != NULL; it = it->next) {
        node_end = node_end->id > it->node->id ? node_end : it->node;
    }
    return node_end;
}

Node *Node::max_edge() const
{
    auto it = edges;
    while (it->next != NULL)
        it = it->next;

    return it ? it->node : NULL;
}

bool Node::has_edges() const
{
    return edges != NULL and max_edge()->id > id;
}

void Node::map_sequence_ids(u32 zero)
{
    auto members = make_members();
    for (auto it = members; it != NULL; it = it->next) {
        it->node->id += zero;
    }
}

Node_Set *seek_members(Node *node, Node_Set *set)
{
    set = node_set_insert(set, node);

    for (auto it = node->edges; it != NULL; it = it->next) {
        if (it->node->id > node->id)
            seek_members(it->node, set);
    }
    return set;
}

Node_Set *Node::make_members()
{
    if (!member_cache)
        member_cache = seek_members(this, member_cache);
    return member_cache;
}

void Node::on_edges_insertion()
{
    node_set_deinit(member_cache);
    member_cache = NULL;
}

Node *Parser::parse()
{
    for (token = source.begin(); token < source.end(); token++) {
        Node *sequence = parse_next_token();
        if (sequence != NULL)
            sequences.push(sequence);
    }

    for (size_t i = 1; i < sequences.len; i++) {
        sequences[0]->merge(sequences[i]);
    }
    return !sequences.empty() ? sequences[0] : NULL;
}

Node *Parser::parse_next_token()
{
    if (token >= source.end())
        return NULL;

    switch (*token) {
    case ' ':
    case '\f':
    case '\n':
    case '\r':
    case '\t':
    case '\v':
        token++;
        return parse_next_token();

    case '_':
        return parse_set(" \v\b\f\t");
    case 'a':
        return parse_set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    case 'o':
        return parse_set("!#$%&()*+,-./:;<=>?@[\\]^`{|}~");
    case 'n':
        return parse_set("0123456789");
    case 'Q':
        return parse_set("\"");
    case 'q':
        return parse_set("'");

    case '[':
        return parse_scope();
    case '^':
        return parse_any();
    case '!':
        return parse_not();
    case '/':
        return parse_dash();
    case '\'':
        return parse_str('\'');
    case '`':
        return parse_str('`');
    case '{':
        return parse_sequence();
    case '|':
        return parse_or();
    case '?':
        return parse_quest();
    case '*':
        return parse_star();
    case '+':
        return parse_plus();
    case '~':
        return parse_wave();

    case '}':
        errorf("unmatched sequence brace, missing '{' token");
    case ']':
        errorf("unmatched scope brace, missing '[' token");
    default:
        errorf("unknown token in regex, none of [_aonQq^'`{{}}!|?*+~]");
    }
}

string Parser::parse_subsequence()
{
    i32 depth = 1;
    const char *begin = token + 1;
    const char *end = begin;

    while (depth > 0 and end < source.end()) {
        switch (*end) {
        case '{':
            depth++;
            break;
        case '}':
            depth--;
            break;
        }
        end++;
    }

    if (depth > 0) {
        errorf("unmatched sequence brace, missing '}' token");
    }

    token = end - 1;
    return string{begin, end - 1};
}

Binary Parser::parse_binary_op(char op)
{
    return Binary{parse_pre_op(op), parse_post_op(op)};
}

Node *Parser::parse_pre_op(char op)
{
    if (sequences.empty()) {
        errorf("missing pre-operand for '%c' operator", op);
    }
    return sequences.pop();
}

Node *Parser::parse_post_op(char op)
{
    token++;
    Node *sequence = parse_next_token();
    if (!sequence) {
        errorf("missing pre-operand for '%c' operator", op);
    }
    return sequence;
}

Node *Parser::parse_set(string set)
{
    auto node = arena->push(Node{});
    node->state.option = Regex_Set;
    node->state.str = set;

    return node;
}

Node *Parser::parse_scope()
{
    if (source.end() - token < 5 or token[0] != '[' or token[2] != '-' or token[4] != ']') {
        errorf("scope does not match the format ('[' ^ '-' ^ ']')");
    }

    auto node = arena->push(Node{});
    node->state.option = Regex_Scope;
    node->state.range[0] = token[1];
    node->state.range[1] = token[3];
    token = &token[4];
    return node;
}

Node *Parser::parse_any()
{
    auto node = arena->push(Node{});
    node->state.option = Regex_Any;
    return node;
}

Node *Parser::parse_str(char quote)
{
    const char *begin = token + 1;
    const char *end = string{begin, source.end()}.find(quote);

    if (end >= source.end()) {
        errorf("unmatched string quote, missing ending '%c' token", quote);
    }
    auto node = arena->push(Node{});
    node->state.option = Regex_Str;
    node->state.str = {begin, end};
    token = end;
    return node;
}

Node *Parser::parse_sequence()
{
    return new_parser(parse_subsequence(), arena).parse();
}

Node *Parser::parse_dash()
{
    auto node = arena->push(Node{});
    node->state.option = Regex_Dash;
    node->state.sequence = parse_post_op('/');
    return node;
}

Node *Parser::parse_not()
{
    auto node = arena->push(Node{});
    node->state.option = Regex_Not;
    node->state.sequence = parse_post_op('!');
    return node;
}

// Control flow structures:
// a: 1st binary operand
// b: 2nd binary operand
// o: unary operand
// $: epsilon
// ^: any
// x: none
// >: edge

Node *Parser::parse_or()
{
    //   > a
    // $
    //   > b
    auto [a, b] = parse_binary_op('|');
    auto sequence = arena->push(Node{});
    sequence->state.option = Regex_Eps;
    sequence->push(a);
    sequence->push(b);

    return sequence;
}

Node *Parser::parse_quest()
{
    //   > o
    // $
    //   > $'
    auto sequence = arena->push(Node{});
    auto no = arena->push(Node{});
    sequence->state.option = Regex_Eps;
    sequence->merge(parse_pre_op('?'));
    sequence->push(no);
    no->state.option = Regex_Eps;

    return sequence;
}

Node *Parser::parse_star()
{
    //   > o > $
    // $
    //   > $'
    auto sequence = arena->push(Node{});
    auto no = arena->push(Node{});
    sequence->state.option = Regex_Eps;
    sequence->merge(parse_pre_op('*'));
    sequence->concat(sequence);
    sequence->push(no);
    no->state.option = Regex_Eps;

    return sequence;
}

Node *Parser::parse_plus()
{
    // o > $ > o
    auto sequence = parse_pre_op('+');
    return sequence->concat(sequence);
}

Node *Parser::parse_wave()
{
    //   > b
    // $
    //   > a > $
    //       > x
    auto [a, b] = parse_binary_op('~');
    auto sequence = arena->push(Node{});
    auto none = arena->push(Node{});
    sequence->state.option = Regex_Eps;
    sequence->push(a);
    sequence->push(b)->concat(sequence);
    a->merge(none);
    none->state.option = Regex_None;

    return sequence;
}

Parser new_parser(string source, Node_Arena *arena)
{
    return Parser{source, &source[0], arena, {}};
}

Match new_match(string expr, u64 index)
{
    Match match = {};
    const char *end = NULL;

    if (index != npos)
        end = &expr[index];
    else
        end = &expr[0];

    match.view = {expr.begin(), end};
    match.next = {end, expr.end()};
    match.ok = index != npos;
    return match;
}

void Regex::deinit()
{
    for (auto it = arena.begin(); it != arena.end(); it++) {
        it->deinit();
    }
}

Match Regex::match(string expr) const
{
    if (!node_head)
        return new_match(expr, npos);
    return new_match(expr, node_head->submit(expr, 0));
}

} // namespace bee::regex

namespace bee
{

Regex compile_regex(string source)
{
    Regex regex = {};
    regex.source = source;
    regex.node_head = new_parser(source, &regex.arena).parse();
    return regex;
}

} // namespace bee

namespace bee::fmt
{
using namespace regex;

const string Graph_Mode = R"(rankdir=LR;bgcolor="#F9F9F9";compound=true{})";
const string Graph_Not_Mode = R"(style=filled;bgcolor="#FBF3F3)";
const string Graph_Dash_Mode = R"(style=filled;bgcolor="#F4FDFF)";

void format_graph(Context *context, Device *dev, Regex *regex);
void format_node(Context *context, Device *dev, Node *node);
void format_define(Context *context, Device *dev, Node *node);
void format_connect(Context *context, Device *dev, Node *node, Node *edge);
void format_subgraph(Context *context, Device *dev, Node *node, string mode);

void format(Context *context, Device *dev, State state)
{
    expect_token(context, dev, *context->verb, "v");

    switch (state.option) {
    case Regex_Monostate:
        return;
    case Regex_Eps:
        return dev->format("&Sigma;");
    case Regex_Any:
        return dev->format("&alpha;");
    case Regex_None:
        return dev->format("&times;");
    case Regex_Not:
        return dev->format("!");
    case Regex_Dash:
        return dev->format("/");
    case Regex_Str:
        return dev->format("%(s:?)", state.str);
    case Regex_Scope:
        return dev->format("[%(c:?)..%(c:?)]", state.range[0], state.range[1]);

    case Regex_Set: {
        switch (state.str.len) {
        case 0:
            return dev->format("[]");
        case 1:
            return dev->format("[%(c:?)]", state.str[0]);
        default: {
            char a = state.str[0];
            char b = state.str[state.str.len - 1];
            return dev->format("[%(c:?)..%(c:?)]", a, b);
        }
        }
    }
    }
}

void format(Context *context, Device *dev, Regex regex)
{
    expect_token(context, dev, *context->verb, "v");
    format_graph(context, dev, &regex);
}

void format_graph(Context *context, Device *dev, Regex *regex)
{
    Node *head = regex->node_head;
    dev->format("strict digraph {\n");

    if (regex->node_head != NULL) {
        dev->format("%s\n", Graph_Mode);
        dev->format(R"(" %(s:?) " [shape="none"]%c)", regex->source, '\n');
        dev->format(R"(" %(s:?) " -> "%p" [label="%v"]%c)", regex->source, (void *)head, head->state, '\n');

        auto members = head->make_members();
        for (auto it = members; it != NULL; it = it->next)
            format_node(context, dev, it->node);
    }

    dev->format("}");
}

void format_subgraph(Context *context, Device *dev, Node *node, string mode)
{
    Node *sequence = node->state.sequence;

    dev->format("subgraph cluster_%p {\n", (void *)node);
    dev->format("%s\n", mode);
    format_define(context, dev, node);
    format_connect(context, dev, node, sequence);

    auto members = node->make_members();

    for (auto it = members; it != NULL; it = it->next) {
        format_node(context, dev, it->node);
    }
    dev->format("}\n");

    Node *end = sequence->end();
    for (auto it = node->edges; it != NULL; it = it->next) {
        format_connect(context, dev, end, it->node);
    }
}

void format_node(Context *context, Device *dev, Node *node)
{
    switch (node->state.option) {
    case Regex_Not:
        return format_subgraph(context, dev, node, Graph_Not_Mode);
    case Regex_Dash:
        return format_subgraph(context, dev, node, Graph_Dash_Mode);

    default:
        format_define(context, dev, node);
        for (auto it = node->edges; it != NULL; it = it->next)
            format_connect(context, dev, node, it->node);
    }
}

void format_define(Context *context, Device *dev, Node *node)
{
    string shape = node->has_edges() ? "square" : "circle";
    dev->format(R"("%p" [shape="%s", label="%d"]%c)", (void *)node, shape, node->id, '\n');
}

void format_connect(Context *context, Device *dev, Node *node, Node *edge)
{
    dev->format(R"("%p" -> "%p" [label="%v"]%c)", (void *)node, (void *)edge, edge->state, '\n');
}

} // namespace bee::fmt
