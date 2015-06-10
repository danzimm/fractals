
#include "worker.hpp"
#include <vector>
#include <iostream>
#include <sys/time.h>

using std::string;
using std::vector;

class DCPrinterWorker;
class PrinterWorker : public virtual ITaskedWorker<string, string> {
  friend class DCPrinterWorker;
  vector<string> _strings;
protected:
  virtual const string unbox() {
    return "";
  }
  virtual Either<double, string> workOn(Index i) {
    if (i < _strings.size()) {
      std::cout << _strings[i] << std::endl;
      return Either<double, string>((double)(i) / (double)(_strings.size()));
    }
    return Either<double, string>(1.);
  }
public:
  PrinterWorker& add_string(const char *s) {
    if (s) {
      _strings.push_back(s);
    }
    return *this;
  }
};

class SPrinterWorker : public PrinterWorker, public ISequencedWorker<string, string> {};
class DPrinterWorker : public PrinterWorker, public IDispatchedSequencedWorker<DISPATCH_QUEUE_PRIORITY_BACKGROUND, string, string> {};
class DCPrinterWorker : public PrinterWorker, public IDispatchedConcurrentWorker<DISPATCH_QUEUE_PRIORITY_BACKGROUND, string, string> {
protected:
  using typename IDispatchedConcurrentWorker<DISPATCH_QUEUE_PRIORITY_BACKGROUND, string, string>::Cardinality;
  virtual const Indexed<string> dispatchingError(uint64_t e, std::string description) {
    return Indexed<string>{e, description};
  }
  virtual Cardinality numberOfTasks() {
    return _strings.size();
  }
};

// TODO: test for matrix multiplications

int main(int argc, const char *argv[]) {
  bool dispatched = argc > 1 ? true : false;
  PrinterWorker *w;
  if (dispatched) {
    if (argc == 2) {
      w = new DPrinterWorker();
    } else {
      w = new DCPrinterWorker();
    }
  } else {
    w = new SPrinterWorker();
  }
  struct timeval t;
  gettimeofday(&t, NULL);
  auto func = [=] (const Indexed<string> s) {
    struct timeval f;
    gettimeofday(&f, NULL);
    printf("Time taken when %sdispatched: %d mus\n", dispatched ? (argc == 2 ? "serially " : "concurrently ") : "not ", f.tv_usec - t.tv_usec);
    exit(0);
  };
  uint64_t i;
  for (i = 0; i < 10000; i++) {
    w->add_string("hello").add_string("world");
  }
  if (dispatched) {
    w->work(func);
    dispatch_main();
  } else {
    w->work();
    struct timeval f;
    gettimeofday(&f, NULL);
    printf("Time taken when %sdispatched: %d mus\n", dispatched ? "" : "not ", f.tv_usec - t.tv_usec);
  }
}

