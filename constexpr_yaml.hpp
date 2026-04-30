#pragma once
#include <algorithm>
#include <charconv>
#include <ctre.hpp>
#include <meta>
#include <ranges>
#include <string>
#include <utility>

namespace {

using namespace std::meta;
using std::vector, std::string_view;
using std::ranges::to, std::views::split, std::views::transform;

// define_aggregate boilerplate to define non member structs
template<info... Ms>
struct Outer {
  struct Inner;
  consteval {
    define_aggregate(^^Inner, {Ms...});
  }
};

template<info... Ms>
using Cls = Outer<Ms...>::Inner;

// For use with substitute to instantiate a reflection of a T with reflections of values Vs
template<typename T, auto... Vs>
constexpr auto construct_from = T{Vs...};

// The above fails for const char * and std::array
template<auto... Vs>
static constexpr std::array<const char*, sizeof...(Vs)> string_array_from{Vs...};

consteval auto strip_whitespace(auto sv) {
  if(!sv.empty()) {
    auto first = sv.find_first_not_of(" \n");
    auto last = sv.find_last_not_of(" \n");
    sv = sv.substr(first, last - first + 1);
  }
  return sv;
}

consteval auto consume_indented_block(auto& text) {
  size_t indent = text.find_first_not_of(' ');
  size_t pos = 0;
  while(pos < text.size()) {
    auto line_end = text.find('\n', pos);
    if(line_end == string_view::npos) break;
    if(line_end == text.size() - 1) break;
    auto line_start = line_end + 1;
    auto line_indent = text.find_first_not_of(' ', line_start) - line_start;
    if(line_indent < indent) {
      auto result = text.substr(0, line_start);
      text.remove_prefix(line_start);
      return result;
    }
    pos = line_start;
  }
  auto result = text;
  text = {};
  return result;
}

consteval auto extract_elements(string_view text, string_view delimiter) {
  return text | split(delimiter) | transform([](auto part) { return strip_whitespace(string_view{part}); }) | to<vector>();
}

consteval auto parse_int(auto s, int base = 10) {
  int v;
  if(!std::from_chars(s.data(), s.data() + s.size(), v, base)) {
    throw "from_chars failed to parse int";
  }
  return v;
}

consteval auto parse_float(auto sign, auto whole, auto fraction, auto exp_sign, auto exp) {
  double result = 0.0;
  if(!whole && !fraction) throw "Expected digits in float";
  if(whole) result = parse_int(whole);
  if(fraction) {
    double f = parse_int(fraction);
    for(int i = 0; i < fraction.size(); ++i) f *= 0.1;
    result += f;
  }
  if(exp) {
    auto exponent = parse_int(exp);
    if(exp_sign == "-") {
      for(int i = 0; i < exponent; ++i) result /= 10;
    } else {
      for(int i = 0; i < exponent; ++i) result *= 10;
    }
  }
  result *= sign == "-" ? -1.0 : 1.0;
  return result;
}

consteval auto parse_string(auto text) {
  using namespace std::string_literals;
  if(text.front() == '\'') {
    if(text.back() != '\'') throw "exected an end quote";
    std::string s{text.substr(1, text.size() - 2)};
    size_t i;
    while((i = s.find("''")) != std::string::npos) {
      s.replace(i, 2, 1, '\'');
    }
    return s;
  } else if(text.front() == '"') {
    if(text.back() != '"') throw "exected an end quote";
    std::string s{text.substr(1, text.size() - 2)};
    for(auto [escapable, c]: {std::pair<char, char>{'0', '\0'}, {'a', '\a'}, {'b', '\b'}, {'t', '\t'}, {'v', '\v'}, {'f', '\f'}, {'"', '"'}, {'/', '/'}, {'n', '\n'}, {'\\', '\\'}}) {
      size_t i;
      while((i = s.find(("\\"s) + escapable)) != std::string::npos) {
        s.replace(i, 2, 1, c);
      }
    }
    // TODO: UTF-8 byte code handling
    return s;
  } else {
    return std::string{text};
  }
}

consteval auto parse(string_view text) {
  if(ctre::match<"^null|Null|NULL|~|$">(text)) return reflect_constant(nullptr);                  // null
  else if(auto [match, boolean] = ctre::match<R"(^(true|True|TRUE|false|False|FALSE)$)">(text)) { // bool
    return reflect_constant(boolean.to_view()[0] == 't' || boolean.to_view()[0] == 'T');
  } else if(auto [match, digits] = ctre::match<"^0x([[:xdigit:]]+)$">(text); match) { // hexadecimal
    return reflect_constant(parse_int(digits, 16));
  } else if(auto [match, digits] = ctre::match<"^0o([0-7]+)$">(text); match) { // octal
    return reflect_constant(parse_int(digits, 8));
  } else if(auto [match, sign, digits] = ctre::match<R"(^([\-+]?)(\d+)$)">(text); match) { // decimal integral
    return reflect_constant(parse_int(digits, 10) * (sign == "-" ? -1 : 1));
  } else if(auto [match, sign, whole, fraction, exp_sign, exp] = ctre::match<R"(^([\-+]?)(\d+)?\.?(\d+)?(?:[eE]([\-+])?(\d+))?$)">(text); match) { // float
    return reflect_constant(parse_float(sign, whole, fraction, exp_sign, exp));
  } else if(auto [match, sign] = ctre::match<R"(([\-+]?)\.(?:inf|Inf|INF))">(text); match) { // inf
    return reflect_constant((sign == "-") ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity());
  } else if(ctre::match<R"(^\.nan|\.NaN|\.NAN|nan$)">(text)) { // nan
    return reflect_constant(std::numeric_limits<double>::quiet_NaN());
  } else { // string
    return reflect_constant_string(parse_string(text));
  }
}

consteval info parse_values(const vector<string_view>& texts) {
  auto values = texts | transform(parse) | to<vector>();
  // TODO: figure out why this works for const char * but not the alternative below
  if(is_array_type(type_of(values[0]))) {
    return substitute(^^string_array_from, values);
  }
  auto array_t = substitute(^^std::array, vector{type_of(values[0]), reflect_constant(values.size())});
  values.insert(values.begin(), array_t);
  return substitute(^^construct_from, values);
}

consteval info parse_node(auto text) {
  if(text.empty()) {
    throw "Expected map values";
  }
  vector<info> members;
  vector<info> values = {^^void};
  auto add_member = [&](string_view key, info val) {
    auto type = type_of(val);
    if(is_array_type(type)) type = ^^const char*;
    auto dms = data_member_spec(type, {.name = key});
    members.push_back(reflect_constant(dms));
    values.push_back(val);
  };
  // for each key:value, parse value as a value, inline value seq, block value seq, or recursively as a node
  while(!text.empty()) {
    auto endl = text.find('\n');
    auto line = text.substr(0, endl);
    text = (endl == string_view::npos) ? "" : text.substr(endl + 1);
    if(auto first = line.find_first_not_of(' '); first == string_view::npos || first == '#') continue;
    size_t indicator = line.find_first_of(":");
    if(indicator == string_view::npos) throw "Expected a map";
    auto key = strip_whitespace(line.substr(0, indicator));
    auto rhs = strip_whitespace(line.substr(indicator + 1));
    if(rhs.empty()) {
      auto body = consume_indented_block(text);
      if(auto first = body.find_first_not_of(" \n"); first != string_view::npos && body[first] == '-') {
        add_member(key, parse_values(extract_elements(body.substr(first + 1), "- ")));
      } else {
        add_member(key, parse_node(body));
      }
    } else {
      if(rhs.starts_with('[') && rhs.ends_with(']')) {
        add_member(key, parse_values(extract_elements(rhs.substr(1, rhs.size() - 2), ",")));
      } else {
        add_member(key, parse(rhs));
      }
    }
  }
  values[0] = substitute(^^Cls, members);
  return substitute(^^construct_from, values);
}

// The text is a compile time constant despite being an argument because the parse_node function is called within a consteval constructor
struct YamlString {
  info rep;
  consteval YamlString(const char* yaml) : rep{parse_node(string_view{yaml})} {}
};
} // namespace

namespace constexpr_yaml {

// Implicitly call the YamlString constructor, and immediately splice it
template<YamlString yaml>
inline constexpr auto parse = [:yaml.rep:];

} // namespace constexpr_yaml
