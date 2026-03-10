<?php
/**
 * @file
 * Entity classes for MedRemind Prescription entity.
 *
 * WHAT IS AN ENTITY?
 *   In Drupal 7, an Entity is the standard way to create structured
 *   content types. Nodes, Users, Taxonomy Terms are all entities.
 *   Entity API module lets us create CUSTOM entities.
 *
 * WHY USE ENTITY?
 *   - Automatic CRUD (Create, Read, Update, Delete)
 *   - Built-in admin UI
 *   - Fieldable (attach fields via UI like nodes)
 *   - Views integration (automatic)
 *   - Rules integration (automatic)
 *   - Follows Drupal standards
 *
 * CUSTOM TABLES vs ENTITY (our module uses BOTH):
 *   Custom Tables: medremind_medications, medremind_log, etc.
 *     → More control, manual SQL, good for simple data
 *   Entity API: medremind_prescriptions
 *     → Auto CRUD, fieldable, Views-ready, less code
 */


/**
 * Prescription Entity class.
 *
 * Extends Entity (from Entity API module).
 * This class represents a single prescription object.
 */
class MedRemindPrescription extends Entity {

  /**
   * Default label for the prescription.
   * Used when Drupal needs to display the entity name.
   */
  protected function defaultLabel() {
    return $this->prescription_number;
  }

  /**
   * Default URI for the prescription.
   * Used for links to the entity.
   */
  protected function defaultUri() {
    return array('path' => 'medremind/prescription/' . $this->prescription_id);
  }
}


/**
 * Prescription Entity Controller.
 *
 * Handles CRUD operations for prescriptions.
 * Extends EntityAPIController (from Entity API module).
 *
 * EntityAPIController gives us for FREE:
 *   - entity_load() / entity_load_single()
 *   - entity_save()
 *   - entity_delete()
 *   - entity_create()
 *   - Database queries
 *   - Caching
 */
class MedRemindPrescriptionController extends EntityAPIController {

  /**
   * Create a new prescription with defaults.
   */
  public function create(array $values = array()) {
    $values += array(
      'prescription_id' => NULL,
      'uid' => 0,
      'med_id' => 0,
      'prescription_number' => '',
      'doctor_name' => '',
      'pharmacy_name' => '',
      'prescribed_date' => REQUEST_TIME,
      'expiry_date' => 0,
      'refills_total' => 0,
      'refills_used' => 0,
      'notes' => '',
      'status' => 1,
      'created' => REQUEST_TIME,
      'updated' => REQUEST_TIME,
    );
    return parent::create($values);
  }
}


/**
 * Prescription UI Controller.
 *
 * Provides admin pages for managing prescriptions.
 * EntityDefaultUIController gives us for FREE:
 *   - Admin list page with operations (edit/delete)
 *   - Add form page
 *   - Edit form page
 *   - Delete confirmation page
 */
class MedRemindPrescriptionUIController extends EntityDefaultUIController {

  /**
   * Override to add custom menu items.
   */
  public function hook_menu() {
    $items = parent::hook_menu();

    // User-facing prescription page
    $items['medremind/prescriptions'] = array(
      'title' => 'Prescriptions',
      'page callback' => 'medremind_prescriptions_user_page',
      'access arguments' => array('use medremind'),
      'type' => MENU_LOCAL_TASK,
      'weight' => 7,
      'file' => 'includes/medremind.prescription.pages.inc',
      'file path' => drupal_get_path('module', 'medremind'),
    );

    return $items;
  }
}