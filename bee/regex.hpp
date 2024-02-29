#ifndef BEE_REGEX_HPP
#define BEE_REGEX_HPP

#include "format.hpp"

namespace bee
{

namespace regex
{
typedef Arena<struct Node, 128> Node_Arena;
typedef Arena<struct Node *, 128> Node_Seq_Arena;

enum Option
{
    Regex_Monostate,
    Regex_Eps,
    Regex_Any,
    Regex_None,
    Regex_Not,
    Regex_Dash,
    Regex_Str,
    Regex_Set,
    Regex_Scope,
};

struct Monostate
{
};

struct State
{
    Option option;
    union {
        Monostate monostate;
        char range[2];
        string str;
        Node *sequence;
    };

    u64 submit(string expr, u64 n) const;
};

struct Node;
struct Node_Set
{
    Node *node;
    Node_Set *next;
};

Node_Set *node_set_insert(Node_Set *set, Node *node);
void node_set_deinit(Node_Set *set);

struct Node
{
    State state;
    Node_Set *edges;
    Node_Set *member_cache;
    u32 id;

    void deinit();
    u64 submit(string expr, u64 n) const;
    Node *push(Node *node);
    Node *merge(Node *node);
    Node *concat(Node *node);
    Node *end();
    Node *max_edge() const;
    bool has_edges() const;

    void map_sequence_ids(u32 zero);
    Node_Set *make_members();
    void when_edges_are_modified();
};

struct Binary
{
    Node *a, *b;
};

struct Parser
{
    string source;
    const char *token;
    Node_Arena *arena;
    Node_Seq_Arena sequences;

    Node *parse();
    Node *parse_next_token();
    string parse_subsequence();

    Binary parse_binary_op(char op);
    Node *parse_pre_op(char op);
    Node *parse_post_op(char op);

    Node *parse_set(string set);
    Node *parse_scope();
    Node *parse_any();
    Node *parse_str(char quote);
    Node *parse_sequence();
    Node *parse_dash();
    Node *parse_not();
    Node *parse_or();
    Node *parse_quest();
    Node *parse_star();
    Node *parse_plus();
    Node *parse_wave();

    [[noreturn]]
    void errorf(string fmt, auto... args)
    {
        using namespace fmt;
        char buf[1024] = {};
        auto ws = new_write_status(buf, sizeof(buf));

        ws = append(ws, "with {\n");
        ws = append(ws, "  %(s)\n", source);
        ws = append(ws, "  %(c:> *)\n", '^', token - source.begin() + 1);
        ws = append(ws, fmt, args...);
        ws = append(ws, "\n}");
        Panic(buf);
    }
};

Parser new_parser(string source, Node_Arena *arena);

struct Match
{
    bool ok;
    string view;
    string next;
};

Match new_match(string expr, u64 index);

struct Regex
{
    string source;
    Node *node_head;
    Node_Arena arena;

    void deinit();
    Match match(string expr) const;
};

}; // namespace regex

using regex::Regex;
Regex compile_regex(string source);

namespace fmt
{
void format(Context *context, Device *dev, Regex regex);
} // namespace fmt

} // namespace bee

#endif
