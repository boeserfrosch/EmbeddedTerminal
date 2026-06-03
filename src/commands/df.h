#ifndef DF_H
#define DF_H

#include "interfaces/ICommand.h"
#include "interfaces/ICommand.h"
#include "interfaces/IStorage.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class df : public ICommand
        {

        public:
            enum ErrorCode
            {
                None = 0,
                FailedToRetrieveMedia = 1
            };

            df(IStorageSystem &storage) : storage_(storage)
            {
            }
            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;

        private:
            IStorageSystem &storage_;
        };
    };
};
#endif // DF_H
