#ifndef H_MENU

#define H_MENU

#include <functional>
#include <string>

class MenuItem {
public:
    MenuItem(const std::string& text, const std::function<void()>& callback) : text(text), callback(callback) {}

    MenuItem(const std::string& text) : text(text), callback(nullptr) {}

    void setText(const std::string& newText) {
      text = newText;
    }

    void setCallback(const std::function<void()>& newCallback) {
      callback = newCallback;
    }

    void executeCallback() {
      if (callback) {
        callback();
      }
    }

    std::string getText() const {
      return text;
    }

private:
  std::string text;
  std::function<void()> callback;
};

#endif