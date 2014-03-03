/**
 * @file
 * jQuery behaviors for the access control kit module UI.
 */

(function ($) {

// Provide vertical tab summaries for access handler settings.
Drupal.behaviors.accessHandlerTabSummaries = {
  attach: function (context) {
    $('fieldset[id^="edit-handlers-"]', context).drupalSetSummary(function(context) {
      // The fieldset id will be in the form "edit-handlers-OBJECT-TYPE"; thus,
      // if we tokenize the id by "-", the type will begin at the third token.
      var tokens = context.id.split('-');
      tokens.splice(0, 2);
      // The object type uses underscores instead of hyphens on form elements.
      var object_type = tokens.join('_');
      var handler = $('input[name="handlers[' + object_type + '][handler]"]:checked', context);
      if (handler.length) {
        var label = $('label[for="' + handler.attr('id') + '"]');
        if (label.length) {
          return Drupal.checkPlain(label.text());
        }
      }
      return '';
    });
  }
};

})(jQuery);
