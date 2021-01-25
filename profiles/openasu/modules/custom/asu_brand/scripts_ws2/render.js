// Hydrate and render components with Preact

(function ($) {
  Drupal.behaviors.asu_brand = {
    attach: function (context, settings) {
      // console.log(JSON.parse(Drupal.settings.asu_brand.navTree));

      var props = {
        navTree: JSON.parse(Drupal.settings.asu_brand.navTree),
        title: Drupal.settings.asu_brand.siteName,
        subtitle: Drupal.settings.asu_brand.siteSubtitle,
        loggedIn: Drupal.settings.asu_brand.isLoggedIn,
        loginLink: Drupal.settings.asu_brand.casLoginLink,
        logoutLink: Drupal.settings.asu_brand.casLogoutLink,
        expandOnHover: Drupal.settings.asu_brand.expandOnHover,
        animateTitle: false
      };

      console.log(props, 'PROPS PASSED');

      componentsLibrary.initHeader(props);

    }
  };
}(jQuery));
