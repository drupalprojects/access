(function ($) {

// Provide vertical tab summaries for access scheme settings.
Drupal.behaviors.accessSchemeTabSummaries = {
  attach: function (context) {
    // The roles tab.
    $('fieldset#edit-roles', context).drupalSetSummary(function(context) {
      var vals = [];
      $("input[name^='role_options']:checked", context).parent().each(function() {
        vals.push(Drupal.checkPlain($(this).text()));
      });
      return vals.join(', ');
    });

    // The object access handlers tab.
    $('fieldset#edit-handlers', context).drupalSetSummary(function(context) {
      var vals = [];
      // In the handlers wrapper, find top-level fieldsets for managing objects.
      $('#access-handlers > .form-wrapper > fieldset[id^="edit-handlers-"]', context).each(function() {
        // The fieldset id will be of the form "edit-handlers-OBJECTTYPE", or
        // "edit-handlers-OBJECTTYPE--NUMBER" (if the handlers wrapper has been
        // rebuild via an AJAX event).  Thus, if we tokenize the id by "-", the
        // object type will be the third token.  We can then use this to find
        // which handler is selected for that object type.
        var tokens = this.id.split('-');
        var handler = $(this).find('select[name="handlers[' + tokens[2] + '][handler]"] option:selected').val();
        // If a handler has been selected, add the object type to the summary.
        if (handler !== '') {
          // The object fieldset's <legend> contains the display name of the
          // object type, but we need to filter out the invisible "Hide" <span>
          // that prefixes it.
          var legend = $(this).children('legend').first().find('.fieldset-legend > .fieldset-title').contents().filter(function() {
            // Grab only the text nodes, not encapsulated elements.
            return this.nodeType == Node.TEXT_NODE;
          }).text();
          vals.push(Drupal.checkPlain(legend));
        }
      });
      return vals.join(', ');
    });
  }
};

})(jQuery);
