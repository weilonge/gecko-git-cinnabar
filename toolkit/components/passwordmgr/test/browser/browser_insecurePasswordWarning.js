"use strict";

const TEST_URL_PATH = "/browser/toolkit/components/passwordmgr/test/browser/";

const WARNING_PATTERN = [{
  key: "INSECURE_FORM_ACTION",
  msg: 'JavaScript Warning: "Password fields present in a form with an insecure (http://) form action. This is a security risk that allows user login credentials to be stolen."'
}, {
  key: "INSECURE_PAGE",
  msg: 'JavaScript Warning: "Password fields present on an insecure (http://) page. This is a security risk that allows user login credentials to be stolen."'
}];

add_task(function* testInsecurePasswordWarning() {
  for (let [origin, testFile, expectWarnings] of [
    // Form action at 127.0.0.1/localhost is considered as a secure case.
    // There should be no INSECURE_FORM_ACTION warning at 127.0.0.1/localhost.
    // This will be fixed at Bug 1261234.
    ["http://127.0.0.1", "form_basic.html", ["INSECURE_FORM_ACTION"]],
    ["http://127.0.0.1", "formless_basic.html", []],
    ["http://example.com", "form_basic.html", ["INSECURE_FORM_ACTION", "INSECURE_PAGE"]],
    ["http://example.com", "formless_basic.html", ["INSECURE_PAGE"]],
    ["https://example.com", "form_basic.html", []],
    ["https://example.com", "formless_basic.html", []],
  ]) {
    let testUrlPath = origin + TEST_URL_PATH + testFile;
    function findWarningPattern(message) {
      return WARNING_PATTERN.find(patternPair => {
        return message.indexOf(patternPair.msg) !== -1;
      });
    }

    function listener(msg) {
      let warning = findWarningPattern(msg.message);
      // Only handle the insecure password related warning messages.
      if (warning) {
        let index = expectWarnings.indexOf(warning.key);

        ok(index !== -1, "Found warning: " + warning.key + " for URL:" + testUrlPath);
        if (index !== -1) {
          // Remove the shown message.
          expectWarnings.splice(index, 1);
        }
      }
    }
    Services.console.registerListener(listener);

    yield BrowserTestUtils.withNewTab({
      gBrowser,
      url: testUrlPath
    }, function*() {
      return new Promise(resolve => {
        setTimeout(() => {
          // Verify if all warnings are shown.
          is(expectWarnings.length, 0, "All warnings are shown. URL:" + testUrlPath);
          Services.console.unregisterListener(listener);
          resolve();
        }, 3000);
      });
    });
  }
});
