#ifndef MOCKGPIOPOLICY_H
#define MOCKGPIOPOLICY_H

#include "../../src/interfaces/IGpioPolicy.h"

class MockGpioPolicy : public EmbeddedTerminal::IGpioPolicy
{
public:
    ETMap<ETString, ETString> aliases;
    ETVector<EmbeddedTerminal::GpioExclusionRule> rules;

    MockGpioPolicy()
    {
        aliases["2"] = "GPIO2";
        aliases["5"] = "GPIO5";
        aliases["PA5"] = "PA5";
        aliases["GPIO2"] = "GPIO2";
        aliases["GPIO5"] = "GPIO5";
    }

    ETString policyName() const override
    {
        return "mock-hybrid-default-deny";
    }

    bool resolveIdentifier(const ETString &input, ETString &resolvedPinId) const override
    {
        auto it = aliases.find(input);
        if (it == aliases.end())
        {
            return false;
        }
        resolvedPinId = it->second;
        return true;
    }

    bool isOperationAllowed(const ETString &resolvedPinId, EmbeddedTerminal::GpioOperation operation, ETString &reason) const override
    {
        for (const auto &rule : rules)
        {
            if (rule.pinId != resolvedPinId)
            {
                continue;
            }

            if (operation == EmbeddedTerminal::GpioOperation::Read && rule.denyRead)
            {
                reason = rule.forced ? "Operation blocked by forced program policy" : "Operation blocked by policy";
                return false;
            }
            if (operation == EmbeddedTerminal::GpioOperation::Write && rule.denyWrite)
            {
                reason = rule.forced ? "Operation blocked by forced program policy" : "Operation blocked by policy";
                return false;
            }
            if (operation == EmbeddedTerminal::GpioOperation::Mode && rule.denyMode)
            {
                reason = rule.forced ? "Operation blocked by forced program policy" : "Operation blocked by policy";
                return false;
            }
            if (operation == EmbeddedTerminal::GpioOperation::Include && rule.denyInclude)
            {
                reason = "Include operation blocked by policy";
                return false;
            }
            if (operation == EmbeddedTerminal::GpioOperation::Exclude && rule.denyExclude)
            {
                reason = "Exclude operation blocked by policy";
                return false;
            }
        }

        reason = "";
        return true;
    }

    bool addExclusion(const EmbeddedTerminal::GpioExclusionRule &rule, ETString &reason) override
    {
        for (auto &existingRule : rules)
        {
            if (existingRule.pinId == rule.pinId)
            {
                if (existingRule.forced)
                {
                    reason = "Pin is forced by program and cannot be overridden";
                    return false;
                }
                existingRule = rule;
                reason = "";
                return true;
            }
        }
        rules.push_back(rule);
        reason = "";
        return true;
    }

    bool removeExclusion(const ETString &resolvedPinId, ETString &reason) override
    {
        for (size_t i = 0; i < rules.size(); i++)
        {
            if (rules[i].pinId == resolvedPinId)
            {
                if (rules[i].forced)
                {
                    reason = "Pin is forced by program and cannot be included";
                    return false;
                }
                rules.erase(rules.begin() + static_cast<long>(i));
                reason = "";
                return true;
            }
        }
        reason = "Exclusion not found";
        return false;
    }

    ETVector<EmbeddedTerminal::GpioExclusionRule> exclusions() const override
    {
        return rules;
    }
};

#endif // MOCKGPIOPOLICY_H