#ifndef A7254FF3_8676_4993_9573_0146D0C0430B
#define A7254FF3_8676_4993_9573_0146D0C0430B

#include <esp_log.h>
#include <list>

namespace cjf
{
  template <typename T, typename TCtx = void *>
  class watchable
  {
  public:
    using valued_changed_func = void (*)(T &item, TCtx ctx);

    watchable();
    bool is_watched() const;
    void watch(valued_changed_func callback, TCtx ctx);
    void unwatch(valued_changed_func callback);
    void notify(T &item) const;

  private:
    struct watcher
    {
      valued_changed_func callback;
      TCtx ctx;

      watcher (valued_changed_func callback, TCtx ctx)
          : callback(callback), ctx(ctx)
      {
      }
    };

    std::list<watcher> _watchers;
  };

  template <typename T, typename TCtx>
  inline watchable<T, TCtx>::watchable()
  {
  }

  template <typename T, typename TCtx>
  inline bool watchable<T, TCtx>::is_watched() const
  {
    return !_watchers.empty();
  }

  template <typename T, typename TCtx>
  inline void watchable<T, TCtx>::watch(valued_changed_func callback, TCtx ctx)
  {
    _watchers.emplace_back(callback, ctx);
  }

  template <typename T, typename TCtx>
  inline void watchable<T, TCtx>::unwatch(valued_changed_func callback)
  {
    _watchers.remove_if([callback](const watcher &w)
                        { return w.callback == callback; });
  }

  template <typename T, typename TCtx>
  inline void watchable<T, TCtx>::notify(T &item) const
  {
    for (const auto &watcher : _watchers)
    {
      watcher.callback(item, watcher.ctx);
    }
  }

}

#endif /* A7254FF3_8676_4993_9573_0146D0C0430B */
