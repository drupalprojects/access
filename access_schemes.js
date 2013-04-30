(function ($) {

Drupal.behaviors.accessSchemes = {
  attach: function (context) {
    // Provide the vertical tab summaries.
    $('fieldset#edit-roles', context).drupalSetSummary(function(context) {
      var vals = [];
      $("input[name^='role_options']:checked", context).parent().each(function() {
        vals.push(Drupal.checkPlain($(this).text()));
      });
      return vals.join(', ');
    });
  }
};

})(jQuery);
