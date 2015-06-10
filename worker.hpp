
#include <functional>
#include <string>
#include <dispatch/dispatch.h>
#include <unistd.h>
#include "utilities.hpp"

#ifndef __worker_hpp
#define __worker_hpp

template<typename T, typename E>
class IWorker {
public:
  using WCompletionHandler = std::function<void(const Optional<T>)>;
  using WProgressHandler = std::function<void(double, const T)>;
  using WFailureHandler = std::function<void(const E)>;
protected:
  virtual const Optional<T> operator!() = 0;
  virtual void _work(WProgressHandler, WCompletionHandler, WFailureHandler) = 0;
public:
  virtual void work(WProgressHandler prog, WCompletionHandler comp, WFailureHandler fail) { _work(prog, comp, fail); }
  virtual void work(WProgressHandler prog, WCompletionHandler comp) { work(prog, comp, NULL); }
  virtual void work(WCompletionHandler comp, WFailureHandler fail) { work(NULL, comp, fail); }
  virtual void work(WCompletionHandler comp) { work(NULL, comp, NULL); }
  virtual void work() { work(NULL, NULL, NULL); }
  virtual ~IWorker() {}
};

template<typename T, typename E>
class ITaskedWorker : public virtual IWorker<Indexed<T>, Indexed<E>> {
protected:
  using Index = typename Indexed<T>::Index;
  virtual Either<double, E> workOn(Index) = 0;
  virtual const T unbox() = 0;
  virtual const Optional<Indexed<T>> operator!() {
    return Optional<Indexed<T>>({0, unbox()});
  }
};

template<typename T, typename E>
class IMutableTaskedWorker : public virtual ITaskedWorker : public virtual ITaskedWorker<T, E> {
public:
  using MTWTask = std::function(Either<double, E>(Optional<T&>));
}

template<typename T, typename E>
class ISequencedWorker : public virtual ITaskedWorker<T, E> {
protected:
  using typename IWorker<Indexed<T>, Indexed<E>>::WCompletionHandler;
  using typename IWorker<Indexed<T>, Indexed<E>>::WProgressHandler;
  using typename IWorker<Indexed<T>, Indexed<E>>::WFailureHandler;
  using Index = typename Indexed<T>::Index;
  virtual void _work(WProgressHandler prog, WCompletionHandler comp, WFailureHandler fail) {
    double p = 0.;
    Index i = 0;
    do {
      Either<double, E> ep = this->workOn(i);
      if (!ep.isLeft) {
        if (fail) fail(Indexed<E>{i, ep.right});
        return;
      }
      p = ep.left;
      if (prog) prog(p, (!*this)(i));
      i++;
    } while (p < 1.);
    if (comp) comp((!*this)(i));
  }
public:
  virtual ~ISequencedWorker() {}
};

template<long I, typename T, typename E>
class IDispatchedWorker : public virtual IWorker<T, E> {
protected:
  using IWorker<T, E>::_work;
public:
  using typename IWorker<T, E>::WCompletionHandler;
  using typename IWorker<T, E>::WProgressHandler;
  using typename IWorker<T, E>::WFailureHandler;
  using IWorker<T, E>::work;
  virtual void work(WProgressHandler prog, WCompletionHandler comp, WFailureHandler fail) {
    dispatch_async(dispatch_get_global_queue(I, 0), ^{
      _work(prog, comp, fail);
    });
  }
  virtual ~IDispatchedWorker() {}
};

template<long I, typename T, typename E>
class IDispatchedSequencedWorker : public virtual ISequencedWorker<T, E>, public virtual IDispatchedWorker<I, Indexed<T>, Indexed<E>> {};

template<long I, typename T, typename E>
class IDispatchedConcurrentWorker : public virtual IDispatchedWorker<I, Indexed<T>, Indexed<E>>, public virtual ITaskedWorker<T, E> {
public:
  using Cardinality = uint64_t;
protected:
  using typename IWorker<Indexed<T>, Indexed<E>>::WCompletionHandler;
  using typename IWorker<Indexed<T>, Indexed<E>>::WProgressHandler;
  using typename IWorker<Indexed<T>, Indexed<E>>::WFailureHandler;
  using Index = typename Indexed<T>::Index;
  virtual Cardinality numberOfTasks() = 0;
  virtual Cardinality numberOfBuckets() { return 0; }
  virtual Cardinality capacityOfBucket(Cardinality i) { return 0; }
  virtual Cardinality maxConcurrentTasks() { return 0; }
  virtual const Indexed<E> dispatchingError(uint64_t e, std::string description) = 0;
  virtual void _work(WProgressHandler prog, WCompletionHandler comp, WFailureHandler fail) {
    dispatch_async(dispatch_get_global_queue(I, 0), ^{
      uint64_t i;
      Cardinality tasks = numberOfTasks();
      Cardinality buckets = numberOfBuckets();
      Cardinality maxConc = maxConcurrentTasks();
      if (buckets == 0) {
        buckets = (Cardinality)sysconf(_SC_NPROCESSORS_ONLN);
        if (maxConc == 0) {
          maxConc = buckets;
        }
      } else {
        if (maxConc == 0) {
          maxConc = (Cardinality)sysconf(_SC_NPROCESSORS_ONLN);
        }
      }
      bool emptyBuckets[buckets];
      Cardinality bucketCapacities[buckets];
      Cardinality totalCapacity = 0;
      Cardinality totalEmpty = 0;
      for (i = 0; i < buckets; i++) {
        Cardinality capa = capacityOfBucket(i);
        if (capa == 0) {
          emptyBuckets[i] = true;
          bucketCapacities[i] = 0;
          totalEmpty++;
        } else {
          emptyBuckets[i] = false;
          bucketCapacities[i] = capa;
          totalCapacity += capa;
        }
      }
      if (totalCapacity < tasks) {
        if (totalEmpty == 0) {
          if (fail) fail(dispatchingError(0, "Every bucket is filled, but not every task will be completed!"));
          return;
        }
        Cardinality nTodo = tasks - totalCapacity;
        Cardinality nPerEmptyBucket = nTodo / totalEmpty;
        Cardinality nLeftOver = nTodo % totalEmpty;
        for (i = 0; i < buckets; i++) {
          if (emptyBuckets[i]) {
            if (nLeftOver > 0) {
              bucketCapacities[i] = nPerEmptyBucket + 1;
              nLeftOver--;
            } else {
              bucketCapacities[i] = nPerEmptyBucket;
            }
            emptyBuckets[i] = false;
          }
        }
      } else if (totalCapacity > tasks) {
        if (fail) fail(dispatchingError(1, "Too many total tasks in buckets, more tasks in the buckets than the total number of tasks to do!"));
        return;
      }
      __block Index taskIndex = 0;
      dispatch_queue_t taskIndexQueue = dispatch_queue_create("im.danz.taskIndexQueue", DISPATCH_QUEUE_SERIAL);
      dispatch_group_t workerGroup = dispatch_group_create();
      auto workBlock = ^{
        while (true) {
          __block Index currentIndex = 0;
          dispatch_sync(taskIndexQueue, ^{
            currentIndex = taskIndex;
            if (taskIndex < tasks)
              taskIndex++;
          });
          if (currentIndex == tasks)
            break;
          this->workOn(currentIndex);
          // TODO: add progress notifications
        }
      };
      for (i = 0; i < maxConc; i++) {
        dispatch_group_async(workerGroup, dispatch_get_global_queue(I, 0), workBlock);
      }
      dispatch_group_wait(workerGroup, DISPATCH_TIME_FOREVER);
      if (comp) comp((!*this)(taskIndex));
    });
  }
};

#endif

