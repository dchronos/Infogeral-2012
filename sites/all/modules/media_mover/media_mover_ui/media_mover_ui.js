
/**
 * @file
 * UI javascript support
 */


Drupal.behaviors.mediaMover = function (context) {
  
  /**
   * Hide all the machine name input fields when the page loads
   */
  $('.machine_name_wrapper').each(function () { 
    // Do not hide if there is an error on the machine name
    if (! $(this).children('div').children('input').hasClass('error')) {  
      $(this).css('display', 'none');
    }
  });
  
  
  /**
   * Toggle the machine name fields
   */
  $('a.machine_name_link').click(function () {
    if (! $(this).parents('.form-item').next('.machine_name_wrapper').hasClass('open')) {
      $(this).parents('.form-item').next('.machine_name_wrapper').show('slow');
      $(this).parents('.form-item').next('.machine_name_wrapper').addClass('open');
    }
    else {
      $(this).parents('.form-item').next('.machine_name_wrapper').removeClass('open');
      $(this).parents('.form-item').next('.machine_name_wrapper').hide('slow');
    }
    return false;
  });
  
  
  /**
   * Handle the step machine names
   */
   $('input.step_machine_name').each(function () {
     // On the page load, check to see if this
     // field has a value already.
     if ($(this).val()) {
       $(this).attr('step_id', '');
     }
     else {
       var step_id = $(this).attr('step_id');
       var name = $('input.step_name[step_id=' + step_id + ']').val()
       name = name.toLowerCase().replace(/[^a-z0-9]/g, '_');
       $(this).val(name);
     }
     
     // @TODO
     // bind a keyup here and remove the step_id 
   });
  
   /**
    * Find all the step names and link changes in these 
    * step names to the machine name generation
    */
  $('input.step_name').bind('keyup', function() {
    var step_id = $(this).attr('step_id');
    var name = $(this).val();
    name = name.toLowerCase().replace(/[^a-z0-9]/g, '_');
    $('input.step_machine_name[step_id=' + step_id + ']').val(name);    
  });    
  
  
    
  // This is some handy code to show and hide form elements that
  // have custom settings
  
  // Identify the form elements that we can use
  var custom_types = ".mm_custom_field_display input:radio:checked, .mm_custom_field_display input:checkbox:checked, .mm_custom_field_display select:selected";
  var watch_types = ".mm_custom_field_display input:radio, .mm_custom_field_display input:checkbox, .mm_custom_field_display select";
  
  // Hide all custom fields on page load
  $('.mm_custom_field_field').each(function() {
    // Use css rather than hide() because these are hidden in a fieldset
    // to start with
    $(this).parent().css('display', 'none');
  });
  
  
  // Search for any custom fields that need to be displayed  
  $(custom_types).each(function () {
    var value = $(this).val();
    // Go up the form hierarchy to find the fieldset for this
    var parent = $(this).parent().parent().parent().parent().parent();
    // Show selected value    
    $('.mm_custom_field_field[mm_custom_field_value="' + value + '"]', parent).parent().css('display', 'block');
  });
  

  // Watch the options for changes and show and hide the 
  // the custom field accordingly
  $(watch_types).each(function () {
    $(this).bind('click', function() {
      var value = $(this).val();
      // Go up the form hierarchy to find the fieldset for this
      var parent = $(this).parent().parent().parent().parent().parent();
      // Show selected value
      $('.mm_custom_field_field[mm_custom_field_value="' + value + '"]', parent).parent().show('slow');
      // Hide all the others
      $('.mm_custom_field_field[mm_custom_field_value!="' + value + '"]', parent).parent().hide('slow');      
    });    
  });
  
  
}