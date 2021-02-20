#include "GifToolsProgress.h"
#include "GifToolsManagedTypes.h"

namespace {
class ProgressTokenImpl : public giftools::ProgressToken {
public:
    ~ProgressTokenImpl() override = default;
    void setReporter(std::unique_ptr<giftools::IProgressReporter>&& instance) override { progressReporter = std::move(instance); }
    void setProgress(double value) override { if (progressReporter) { progressReporter->reportProgress(value); } }
    
private:
    std::unique_ptr<giftools::IProgressReporter> progressReporter = {};
} progressToken;

class CancellationTokenImpl : public giftools::CancellationToken {
public:
    ~CancellationTokenImpl() override = default;
    void setSource(std::unique_ptr<giftools::ICancellationSource>&& source) override { cancellationSource = std::move(source); }
    bool isCancelled() const override { return cancellationSource ? cancellationSource->shouldCancel() : false; }

private:
    std::unique_ptr<giftools::ICancellationSource> cancellationSource = {};
} cancellationToken;
}

namespace giftools {
ProgressToken* getMutableProgressToken() { return &progressToken; }
const ProgressToken* getProgressToken() { return &progressToken; }
CancellationToken* getMutableCancellationToken() { return &cancellationToken; }
const CancellationToken* getCancellationToken() { return &cancellationToken; }
}
