#include <boost/ut.hpp>
#include <constexpr_yaml.hpp>
#include <iostream>

template<std::meta::info type>
void print_struct(const auto& obj, std::string prefix = "") {
  template for(constexpr auto member: define_static_array(nonstatic_data_members_of(type, std::meta::access_context::current()))) {
    using MemberT = typename[:type_of(member):];
    std::cout << prefix;
    if constexpr(std::is_same_v<std::string, MemberT>) {
      std::cout << identifier_of(member) << ": \"" << obj.[:member:] << "\"\n";
    } else if constexpr(std::meta::has_template_arguments(type_of(member)) && std::meta::template_of(type_of(member)) == ^^std::array) {
      std::cout << identifier_of(member) << ": [" << obj.[:member:][0];
      for(auto i = begin(obj.[:member:]) + 1; i < end(obj.[:member:]); ++i) {
        std::cout << ", " << *i;
      }
      std::cout << "]\n";
    } else if constexpr(is_class_type(type_of(member))) {
      std::cout << identifier_of(member) << ":\n";
      print_struct<^^MemberT>(obj.[:member:], prefix + "\t");
    } else {
      std::cout << identifier_of(member) << ": " << obj.[:member:] << "\n";
    }
  }
}

constexpr const char data[] = {
#embed "test.yaml"
    , 0};

int main() {
  constexpr auto test = constexpr_yaml::parse<data>;
  print_struct<^^decltype(test)>(test);
  using namespace boost::ut;
  using namespace std::string_view_literals;

  "yaml"_test = [test] {
    expect(test.integrals.null == nullptr);
    expect(test.integrals.hex == 0x10);
    expect(test.integrals.octal == 010);
    expect(test.integrals.decimal == 10);
    expect(test.integrals.plus == 10);
    expect(test.integrals.minus == -10);
    expect(test.fractions.typical == 1.0_d);
    expect(test.fractions.plus == 1.0_d);
    expect(test.fractions.minus == -1.0_d);
    expect(test.fractions.start == 0.1_d);
    expect(test.fractions.end == 1.0_d);
    expect(test.fractions.sci == 1000.0_d);
    expect(test.fractions.all == -1234.0_d);
    expect(test.fractions.plus_inf == std::numeric_limits<double>::infinity());
    expect(test.fractions.minus_inf == -std::numeric_limits<double>::infinity());
    expect(std::isnan(test.fractions.nan));
    expect(test.boolean.True == true);
    expect(test.boolean.False == false);
    expect(test.strings.unquoted == "Hello World!"sv);
    expect(test.strings.single_quoted == "Hello World!"sv);
    expect(test.strings.double_quoted == "Hello World!"sv);
    expect(test.strings.special_single == "'\\$#@!'"sv);
    expect(test.strings.special_double == "'\t'"sv);
    expect(test.sequences.flow.string[0] == "First"sv);
    expect(test.sequences.flow.integral[2] == 2);
    expect(test.sequences.block.string[1] == "Second Element"sv);
    expect(test.sequences.block.integral[0] == 0);
    expect(test.deeply.nested.map.mixed_sequences.names[0] == "liz"sv);
    expect(test.deeply.nested.map.mixed_sequences.names[1] == "elizabeth"sv);
    expect(test.deeply.nested.map.mixed_sequences.names[2] == "lizzy"sv);
    expect(test.deeply.nested.map.mixed_sequences.numbers[0] == 24);
    expect(test.deeply.nested.map.mixed_sequences.numbers[1] == 13); // 0xD
    expect(test.deeply.nested.map.mixed_sequences.numbers[2] == 6);  // 0o6
    expect(test.deeply.nested.map.mixed_sequences.numbers[3] == -11);
  };

  return 0;
}
