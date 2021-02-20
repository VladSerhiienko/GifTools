#pragma once
#include "GifToolsManagedObject.h"

namespace giftools {

class IProgressReporter {
public:
    virtual ~IProgressReporter() = default;
    virtual void reportProgress(double value) = 0;
};
class ICancellationSource {
public:
    virtual ~ICancellationSource() = default;
    virtual bool shouldCancel() const = 0;
};

class ProgressToken {
public:
    virtual ~ProgressToken() = default;
    virtual void setReporter(std::unique_ptr<IProgressReporter>&& reporter) = 0;
    virtual void setProgress(double value) = 0;
};

class CancellationToken {
public:
    virtual ~CancellationToken() = default;
    virtual void setSource(std::unique_ptr<ICancellationSource>&& reporter) = 0;
    virtual bool isCancelled() const = 0;
};

ProgressToken* getMutableProgressToken();
const ProgressToken* getProgressToken();

CancellationToken* getMutableCancellationToken();
const CancellationToken* getCancellationToken();

} // namespace giftools
