#ifndef IGPIOPOLICY_H
#define IGPIOPOLICY_H

#include "../ETTypes.h"

namespace EmbeddedTerminal
{
    enum class GpioOperation
    {
        Read,
        Write,
        Mode,
        Include,
        Exclude
    };

    struct GpioExclusionRule
    {
        ETString pinId;
        bool denyRead = true;
        bool denyWrite = true;
        bool denyMode = true;
        bool denyInclude = false;
        bool denyExclude = false;
        bool forced = false;
        bool passwordProtected = false;
    };

    class IGpioPolicy
    {
    public:
        virtual ~IGpioPolicy() {}

        virtual ETString policyName() const = 0;
        virtual bool resolveIdentifier(const ETString &input, ETString &resolvedPinId) const = 0;
        virtual bool isOperationAllowed(const ETString &resolvedPinId, GpioOperation operation, ETString &reason) const = 0;
        virtual bool addExclusion(const GpioExclusionRule &rule, ETString &reason) = 0;
        virtual bool removeExclusion(const ETString &resolvedPinId, ETString &reason) = 0;
        virtual ETVector<GpioExclusionRule> exclusions() const = 0;
    };
}

#endif // IGPIOPOLICY_H