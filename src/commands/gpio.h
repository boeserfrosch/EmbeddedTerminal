#ifndef GPIO_H
#define GPIO_H

#include "interfaces/ICommand.h"
#include "interfaces/IGpioInterface.h"
#include "interfaces/IGpioPolicy.h"
#include "interfaces/IGpioAuth.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class gpio : public ICommand
        {
        public:
            gpio(IGpioInterface &gpio, IGpioPolicy &policy, IGpioAuth *auth = nullptr) : gpio_(gpio), policy_(policy), auth_(auth)
            {
            }

            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;

        private:
            ETString run_(const ETVector<ETString> &arguments, CommandContext *context, int &exitCode);
            bool isAuthenticated_(CommandContext *context) const;
            bool requiresAuthentication_(const GpioExclusionRule &rule) const;
            ETString parseOperationTokens_(const ETVector<ETString> &tokens, size_t startIndex, GpioExclusionRule &rule);
            bool resolveAndAuthorize_(const ETString &inputPin, GpioOperation operation, ETString &resolvedPin, ETString &error);
            ETString formatRule_(const GpioExclusionRule &rule) const;
            ETVector<ETString> parsePins_(const ETString &rawPins) const;
            bool resolveAndValidatePin_(const ETString &inputPin, ETString &resolvedPin, ETString &error) const;
            uint64_t nowMs_() const;

            IGpioInterface &gpio_;
            IGpioPolicy &policy_;
            IGpioAuth *auth_;

        protected:
            const char *SESSION_KEY_AUTHENTICATED = "gpio__authenticated";
            const char *SESSION_KEY_AUTH_UNTIL_MS = "gpio__auth_until_ms";
        };
    }
}

#endif // GPIO_H