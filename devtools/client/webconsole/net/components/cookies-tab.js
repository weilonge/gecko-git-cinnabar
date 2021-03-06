/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
"use strict";

const React = require("devtools/client/shared/vendor/react");
const NetInfoGroupList = React.createFactory(require("./net-info-group-list"));
const Spinner = React.createFactory(require("./spinner"));

// Shortcuts
const DOM = React.DOM;
const PropTypes = React.PropTypes;

/**
 * This template represents 'Cookies' tab displayed when the user
 * expands network log in the Console panel. It's responsible for rendering
 * sent and received cookies.
 */
var CookiesTab = React.createClass({
  propTypes: {
    actions: PropTypes.shape({
      requestData: PropTypes.func.isRequired
    }),
    data: PropTypes.object.isRequired,
  },

  displayName: "CookiesTab",

  render() {
    let actions = this.props.actions;
    let file = this.props.data;

    let cookies = file.request.cookies;
    if (!cookies || !cookies.length) {
      // TODO: use async action objects as soon as Redux is in place
      actions.requestData("requestCookies");

      return (
        Spinner()
      );
    }

    // The cookie panel displays two groups of cookies:
    // 1) Response Cookies
    // 2) Request Cookies
    let groups = [{
      key: "responseCookies",
      name: Locale.$STR("responseCookies"),
      params: file.response.cookies
    }, {
      key: "requestCookies",
      name: Locale.$STR("requestCookies"),
      params: file.request.cookies
    }];

    return (
      DOM.div({className: "cookiesTabBox"},
        DOM.div({className: "panelContent"},
          NetInfoGroupList({
            groups: groups
          })
        )
      )
    );
  }
});

// Exports from this module
module.exports = CookiesTab;
