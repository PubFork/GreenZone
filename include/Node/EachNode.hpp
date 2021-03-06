/*
 * EachNode.h
 *
 *  Created on: 2014
 *      Author: jc
 */

#pragma once

#include <Common.hpp>
#include <Context/Context.hpp>
#include <Exception.hpp>
#include <Parser/ExpressionParser.hpp>
#include <Parser/Fragment.hpp>
#include <Node/Node.hpp>

#include <iostream>
#include <regex>


namespace GreenZone
{
	class Writer;

	class EachNode : public Node
	{
	public:
		EachNode() : Node(true) {}

		virtual void render(Writer * stream, Context * context) const
		{
			ExpressionParser parser(context);
			json11::Json container = parser.parse(m_container);
			if (!(container.is_array() || container.is_object()))
			{
				throw Exception(container.dump() + " is not iterable");
			}

			json11::Json::object prototype = context->json().object_items();

			std::shared_ptr< Context > newContext(new Context(prototype));
			if (container.type() == json11::Json::ARRAY)
			{
				json11::Json::array items = container.array_items();
				for (auto const & item : items)
				{
					prototype[m_vars[0]] = item;
					newContext->setJson(json11::Json(prototype));
					renderChildren(stream, newContext.get());
				}
			}
			else if (container.type() == json11::Json::OBJECT)
			{
				json11::Json::object items = container.object_items();
				for (auto const & item : items)
				{
					prototype[m_vars[0]] = item.first;
					if (m_vars.size() > 1)
					{
						prototype[m_vars[1]] = item.second;
					}
					newContext->setJson(json11::Json(prototype));
					renderChildren(stream, newContext.get());
				}
			}
		}
		virtual void processFragment(Fragment const * fragment)
		{
			static std::regex const splitter(R"(^for\s+(\w[a-zA-Z0-9 _,]*) \s*in\s+(.+)$)");
			std::smatch match;
			std::string cleaned = fragment->clean();
			if (!std::regex_match(cleaned, match, splitter))
			{
				throw TemplateSyntaxError(fragment->clean());
			}
			std::string vars = match[1];
			static std::regex const varNameExtractor(R"(\s*,\s*)");
			std::sregex_token_iterator varNamesIter(vars.begin(), vars.end(), varNameExtractor, -1);
			std::vector< std::string > possibleVars;
			static std::sregex_token_iterator const end;
			std::copy(varNamesIter, end, std::back_inserter(possibleVars));
			if (std::find(possibleVars.begin(), possibleVars.end(), "") != possibleVars.end())
			{
				throw TemplateSyntaxError(vars);
			}
			m_vars = possibleVars;
			m_container = match[2];
		}

		virtual void exitScope(std::string const & endTag)
		{
			if (endTag != "endfor")
				throw TemplateSyntaxError(endTag);
		}
		virtual std::string name() const { return "For"; }


		virtual ~EachNode(){}

	protected:
		std::string m_container;
		std::vector< std::string > m_vars;
	};

} /* namespace RedZone */

