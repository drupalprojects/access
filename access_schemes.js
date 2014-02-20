/**
 * @file
 * jQuery behaviors for the access control kit module UI.
 */

(function ($) {

// Provide vertical tab summaries for access handler settings.
Drupal.behaviors.accessHandlerTabSummaries = {
  attach: function (context) {
    $('fieldset[id^="edit-handlers-"]', context).drupalSetSummary(function(context) {
      // The fieldset id will be of the form "edit-handlers-OBJECT_TYPE"; thus,
      // if we tokenize the id by "-", the object type will be the third token.
      var tokens = context.id.split('-');
      var handler = $('input[name="handlers[' + tokens[2] + '][handler]"]:checked', context);
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
