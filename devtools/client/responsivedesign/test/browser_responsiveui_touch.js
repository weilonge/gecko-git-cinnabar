/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

"use strict";

const TEST_URI = "http://mochi.test:8888/browser/devtools/client/" +
                 "responsivedesign/test/touch.html";
const domViewportEnabled = "dom.meta-viewport.enabled";

add_task(function*() {
  let tab = yield addTab(TEST_URI);
  let {rdm} = yield openRDM(tab);
  let {inspector, view} = yield openComputedView();
  yield selectNode("div", inspector);
  yield testWithNoTouch();
  yield rdm.enableTouch();
  yield testWithTouch();
  yield rdm.disableTouch();
  yield testWithNoTouch();
  yield closeRDM(rdm);
});

function* testWithNoTouch() {
  let div = content.document.querySelector("div");
  let x = -1, y = -1;
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mousemove", isSynthesized: false }, gBrowser.selectedBrowser);
  x = 2; y = 2;
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mousemove", isSynthesized: false }, gBrowser.selectedBrowser);
  yield (function (){
    return new Promise(resolve => {
      setTimeout(function () {
        resolve();
      },1000);
    });
  })();
  is(div.style.backgroundColor, "red", "mouseenter or mouseover should work");
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mousedown", isSynthesized: false }, gBrowser.selectedBrowser);
  x += 20; y += 10;
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mousemove", isSynthesized: false }, gBrowser.selectedBrowser);
  is(div.style.transform, "none", "touch shouldn't work");
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mouseup", isSynthesized: false }, gBrowser.selectedBrowser);
  x = -1; y = -1;
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mousemove", isSynthesized: false }, gBrowser.selectedBrowser);
  is(div.style.backgroundColor, "", "mouseout or mouseleave should work");

  yield synthesizeClick();
  is(div.dataset.isDelay, "false", "300ms delay between touch events and mouse events should not work");
}

function* testWithTouch() {
  let div = content.document.querySelector("div");
  let x = -1, y = -1;
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mousemove", isSynthesized: false }, gBrowser.selectedBrowser);
  x = 2; y = 2;
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mousemove", isSynthesized: false }, gBrowser.selectedBrowser);
  is(div.style.backgroundColor, "", "mouseenter or mouseover should not work");
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mousedown", isSynthesized: false }, gBrowser.selectedBrowser);
  x += 20; y += 10;
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mousemove", isSynthesized: false }, gBrowser.selectedBrowser);
  is(div.style.transform, "translate(20px, 10px)", "touch should work");
  yield BrowserTestUtils.synthesizeMouse("div", x, y,
        { type: "mouseup", isSynthesized: false }, gBrowser.selectedBrowser);
  is(div.style.transform, "none", "touchend event should work");
  yield BrowserTestUtils.waitForEvent(div, "mousedown", false);
  yield testWithMetaViewportEnabled();
  yield testWithMetaViewportDisabled();
}

function* testWithMetaViewportEnabled() {
  yield pushPrefs([domViewportEnabled, true]);
  let meta = content.document.querySelector('meta[name=viewport]');
  let div = content.document.querySelector("div");

  meta.content = "";
  yield synthesizeClick();
  yield BrowserTestUtils.waitForEvent(div, "mousedown", false);
  is(div.dataset.isDelay, "true", "300ms delay between touch events and mouse events should work");

  meta.content = "user-scalable=no";
  yield synthesizeClick();
  is(div.dataset.isDelay, "false", "300ms delay between touch events and mouse events should not work");

  meta.content = "minimum-scale=maximum-scale";
  yield synthesizeClick();
  is(div.dataset.isDelay, "false", "300ms delay between touch events and mouse events should not work");

  meta.content = "width=device-width";
  yield synthesizeClick();
  is(div.dataset.isDelay, "false", "300ms delay between touch events and mouse events should not work");
}

function* testWithMetaViewportDisabled() {
  yield pushPrefs([domViewportEnabled, false]);
  let meta = content.document.querySelector('meta[name=viewport]');
  let div = content.document.querySelector("div");

  meta.content = "";
  yield synthesizeClick();
  yield BrowserTestUtils.waitForEvent(div, "mousedown", false);
  is(div.dataset.isDelay, "true", "300ms delay between touch events and mouse events should work");
}

function* synthesizeClick() {
  yield BrowserTestUtils.synthesizeMouseAtCenter("div", { type: "mousedown", isSynthesized: false },
        gBrowser.selectedBrowser);
  yield BrowserTestUtils.synthesizeMouseAtCenter("div", { type: "mouseup", isSynthesized: false },
        gBrowser.selectedBrowser);
}

function pushPrefs(...aPrefs) {
  let deferred = promise.defer();
  SpecialPowers.pushPrefEnv({"set": aPrefs}, deferred.resolve);
  return deferred.promise;
}
