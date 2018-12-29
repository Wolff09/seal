#pragma once

#include <memory>
#include <deque>
#include <unordered_map>
// TODO: import string
#include "antlr4-runtime.h"
#include "CoLaBaseVisitor.h"
#include "cola/ast.hpp"


namespace cola {

	class TypeBuilder : public cola::CoLaBaseVisitor {
		private:
			std::unordered_map<std::string, std::unique_ptr<Type>> _declared_types;
			std::unordered_map<std::string, std::reference_wrapper<const Type>> _all_types;
			bool _created = false;
			Type* _currentType;

			std::string mk_name(std::string name) {
				return name + "*";
			}

		public:
			struct TypeInfo {
				std::vector<std::unique_ptr<Type>> types;
				std::unordered_map<std::string, std::reference_wrapper<const Type>> lookup;
			};

			static std::unique_ptr<TypeInfo> build(cola::CoLaParser::ProgramContext* context) {
				TypeBuilder builder;
				context->accept(&builder);
				auto typeinfo = std::make_unique<TypeInfo>();
				typeinfo->lookup = std::move(builder._all_types);
				typeinfo->types.reserve(builder._declared_types.size());
				for (auto& kvpair : builder._declared_types) {
					typeinfo->types.push_back(std::move(kvpair.second));
				}
				return typeinfo;
			}

			antlrcpp::Any visitProgram(cola::CoLaParser::ProgramContext* context) override {
				// add primitive types
				_all_types.insert({{ Type::void_type().name, Type::void_type() }});
				_all_types.insert({{ Type::bool_type().name, Type::bool_type() }});
				_all_types.insert({{ Type::data_type().name, Type::data_type() }});
				_all_types.insert({{ Type::null_type().name, Type::null_type() }});

				for (auto structContext : context->struct_decl()) {
					structContext->accept(this);
				}
				
				_created = true;
				for (auto structContext : context->struct_decl()) {
					structContext->accept(this);
				}

				return nullptr;
			}

			antlrcpp::Any visitStruct_decl(cola::CoLaParser::Struct_declContext* context) override {
				if (!_created) {
					// find all type declarations
					std::string name = mk_name(context->name->getText());
					if (_all_types.count(name) != 0) {
						throw std::logic_error("Duplicate type declaration: type '" + name + "' already defined.");
					} else {
						auto new_type = std::make_unique<Type>(name, Sort::PTR);
						_all_types.insert({{ name, std::cref(*new_type.get()) }});
						_declared_types[name] = std::move(new_type);
					}

				} else {
					// handle type declarations (add fields)
					std::string name = mk_name(context->name->getText());
					if (_declared_types.count(name) == 0) {
						throw std::logic_error("Compilation error: expected type to exist.");
					}
					_currentType = _declared_types.at(name).get();
					for (auto fieldContext : context->field_decl()) {
						fieldContext->accept(this);
					}
				}
				return nullptr;
			}

			const Type& lookup(std::string name) {
				if (_all_types.count(name) == 0) {
					throw std::logic_error("Field declaration of unknown type name '" + name + "'.");
				}
				const Type& type = _all_types.at(name).get();
				return type;
			}

			antlrcpp::Any visitField_decl(cola::CoLaParser::Field_declContext* context) override {
				assert(_currentType);
				const Type& fieldType = lookup(context->type()->accept(this).as<std::string>());
				for (auto& token : context->names) {
					std::string name = token->getText();
					if (_currentType->fields.count(name) != 0) {
						throw std::logic_error("Duplicate field declaration: field with name '" + name + "' already exists.");
					}
					auto wrapper = std::cref(fieldType);
					_currentType->fields.insert({{ name, wrapper }});
				}
				return nullptr;
			}

			antlrcpp::Any visitTypeValue(cola::CoLaParser::TypeValueContext* context) override {
				std::string name = context->name->accept(this).as<std::string>();
				return name;
			}

			antlrcpp::Any visitTypePointer(cola::CoLaParser::TypePointerContext* context) override {
				std::string name = context->name->accept(this).as<std::string>() + "*";
				return name;
			}

			antlrcpp::Any visitNameVoid(cola::CoLaParser::NameVoidContext* /*context*/) override {
				std::string name = "void";
				return name;
			}

			antlrcpp::Any visitNameBool(cola::CoLaParser::NameBoolContext* /*context*/) override {
				std::string name = "bool";
				return name;
			}

			antlrcpp::Any visitNameInt(cola::CoLaParser::NameIntContext* /*context*/) override {
				throw std::logic_error("Type 'int' not supported. Use 'data_t' instead.");
			}

			antlrcpp::Any visitNameData(cola::CoLaParser::NameDataContext* /*context*/) override {
				std::string name = "data_t";
				return name;
			}

			antlrcpp::Any visitNameIdentifier(cola::CoLaParser::NameIdentifierContext* context) override {
				std::string name = context->Identifier()->getText();
				return name;
			}
	};

} // namespace cola
