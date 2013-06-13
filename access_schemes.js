/**
 * @file
 * jQuery behaviors for the access control kit module UI.
 */

(function ($) {

// Provide vertical tab summaries for access handler settings.
Drupal.behaviors.accessHandlerTabSummaries = {
  attach: function (context) {
    $('fieldset[id^="edit-handlers-"]', context).drupalSetSummary(function(context) {
      // The fieldset id will be of the form "edit-handlers-OBJECTTYPE"; thus,
      // if we tokenize the id by "-", the object type will be the third token.
      var tokens = context.id.split('-');
      var handler = $('select[name="handlers[' + tokens[2] + '][handler]"] option:selected', context);
      if (handler.val() == '') {
        return Drupal.t('Not managed');
      }
      else {
        return Drupal.checkPlain(handler.text());
      }
    });
  }
};

})(jQuery);
