#ifndef DF_H
#define DF_H

#include "Terminal.h"
#include "interfaces/ICommand.h"
#include "interfaces/IStorage.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class df : public ICommand
        {

        public:
            df(IStorageSystem &storage) : storage_(storage)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            IStorageSystem& storage_;
        };
    };
};
#endif // DF_H
