#pragma once

#include "interfaces/IGpioPolicy.h"
#include "ETTypes.h"

#include <cctype>

namespace EmbeddedTerminal
{
    class ConfigurableGpioPolicy : public IGpioPolicy
    {
    public:
        ConfigurableGpioPolicy(const ETString &policyName,
                               bool defaultAllow,
                               const ETString &allowedPinsCsv = "",
                               const ETString &forcedExcludedPinsCsv = "")
            : policyName_(policyName), defaultAllow_(defaultAllow)
        {
            parseAllowedPins_(allowedPinsCsv);
            parseForcedExclusions_(forcedExcludedPinsCsv);
        }

        ETString policyName() const override
        {
            return policyName_;
        }

        bool resolveIdentifier(const ETString &input, ETString &resolvedPinId) const override
        {
            auto normalized = normalizePinId_(input);
            if (normalized.empty())
            {
                return false;
            }

            resolvedPinId = normalized;
            return true;
        }

        bool isOperationAllowed(const ETString &resolvedPinId, GpioOperation operation, ETString &reason) const override
        {
            auto forcedRule = findRule_(resolvedPinId, true);
            if (forcedRule != nullptr && deniesOperation_(*forcedRule, operation))
            {
                reason = "Operation blocked by forced program policy";
                return false;
            }

            auto dynamicRule = findRule_(resolvedPinId, false);
            if (dynamicRule != nullptr && deniesOperation_(*dynamicRule, operation))
            {
                reason = "Operation blocked by policy";
                return false;
            }

            if (dynamicRule != nullptr && isExplicitAllowRule_(*dynamicRule))
            {
                reason = "";
                return true;
            }

            if (!allowedPins_.empty())
            {
                if (allowedPins_.find(resolvedPinId) == allowedPins_.end())
                {
                    reason = "Pin is not in board allowlist";
                    return false;
                }
                reason = "";
                return true;
            }

            if (!defaultAllow_)
            {
                reason = "Pin is denied by default policy";
                return false;
            }

            reason = "";
            return true;
        }

        bool addExclusion(const GpioExclusionRule &rule, ETString &reason) override
        {
            if (findRule_(rule.pinId, true) != nullptr)
            {
                reason = "Pin is forced by program and cannot be overridden";
                return false;
            }

            for (auto &existing : sessionRules_)
            {
                if (existing.pinId == rule.pinId)
                {
                    existing = rule;
                    reason = "";
                    return true;
                }
            }

            sessionRules_.push_back(rule);
            reason = "";
            return true;
        }

        bool removeExclusion(const ETString &resolvedPinId, ETString &reason) override
        {
            if (findRule_(resolvedPinId, true) != nullptr)
            {
                reason = "Pin is forced by program and cannot be included";
                return false;
            }

            for (size_t i = 0; i < sessionRules_.size(); i++)
            {
                if (sessionRules_[i].pinId == resolvedPinId)
                {
                    sessionRules_.erase(sessionRules_.begin() + i);
                    reason = "";
                    return true;
                }
            }

            reason = "Exclusion not found";
            return false;
        }

        ETVector<GpioExclusionRule> exclusions() const override
        {
            ETVector<GpioExclusionRule> out = forcedRules_;
            out.insert(out.end(), sessionRules_.begin(), sessionRules_.end());
            return out;
        }

    private:
        static ETString normalizePinId_(const ETString &raw)
        {
            ETString value = raw.trim();
            if (value.empty())
            {
                return "";
            }

            std::string upper = static_cast<std::string>(value);
            for (auto &ch : upper)
            {
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }

            bool onlyDigits = true;
            for (auto ch : upper)
            {
                if (!std::isdigit(static_cast<unsigned char>(ch)))
                {
                    onlyDigits = false;
                    break;
                }
            }
            if (onlyDigits)
            {
                return ETString("GPIO") + ETString(upper);
            }

            if (upper.rfind("GPIO", 0) == 0)
            {
                ETString suffix = upper.substr(4);
                if (suffix.empty())
                {
                    return "";
                }
                for (size_t i = 0; i < suffix.length(); i++)
                {
                    if (!std::isdigit(static_cast<unsigned char>(suffix[i])))
                    {
                        return "";
                    }
                }
                return ETString("GPIO") + suffix;
            }

            if (upper.length() >= 3 && std::isalpha(static_cast<unsigned char>(upper[0])) && std::isalpha(static_cast<unsigned char>(upper[1])))
            {
                for (size_t i = 2; i < upper.length(); i++)
                {
                    if (!std::isdigit(static_cast<unsigned char>(upper[i])))
                    {
                        return "";
                    }
                }
                return ETString(upper);
            }

            return "";
        }

        void parseAllowedPins_(const ETString &csv)
        {
            auto entries = split(csv, ",");
            for (const auto &entry : entries)
            {
                ETString normalized = normalizePinId_(entry);
                if (!normalized.empty())
                {
                    allowedPins_[normalized] = true;
                }
            }
        }

        void parseForcedExclusions_(const ETString &csv)
        {
            auto entries = split(csv, ",");
            for (const auto &entry : entries)
            {
                ETString normalized = normalizePinId_(entry);
                if (normalized.empty())
                {
                    continue;
                }

                GpioExclusionRule rule;
                rule.pinId = normalized;
                rule.denyRead = true;
                rule.denyWrite = true;
                rule.denyMode = true;
                rule.denyInclude = true;
                rule.denyExclude = true;
                rule.forced = true;
                rule.passwordProtected = false;
                forcedRules_.push_back(rule);
            }
        }

        const GpioExclusionRule *findRule_(const ETString &pinId, bool forced) const
        {
            const auto &source = forced ? forcedRules_ : sessionRules_;
            for (const auto &rule : source)
            {
                if (rule.pinId == pinId)
                {
                    return &rule;
                }
            }
            return nullptr;
        }

        static bool deniesOperation_(const GpioExclusionRule &rule, GpioOperation operation)
        {
            if (operation == GpioOperation::Read)
                return rule.denyRead;
            if (operation == GpioOperation::Write)
                return rule.denyWrite;
            if (operation == GpioOperation::Mode)
                return rule.denyMode;
            if (operation == GpioOperation::Include)
                return rule.denyInclude;
            if (operation == GpioOperation::Exclude)
                return rule.denyExclude;
            return false;
        }

        static bool isExplicitAllowRule_(const GpioExclusionRule &rule)
        {
            return !rule.denyRead && !rule.denyWrite && !rule.denyMode && !rule.denyInclude && !rule.denyExclude;
        }

        ETString policyName_;
        bool defaultAllow_ = false;
        ETMap<ETString, bool> allowedPins_;
        ETVector<GpioExclusionRule> forcedRules_;
        ETVector<GpioExclusionRule> sessionRules_;
    };
}