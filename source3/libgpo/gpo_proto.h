
/* The following definitions come from libgpo/gpo_filesync.c  */

NTSTATUS gpo_copy_file(TALLOC_CTX *mem_ctx,
		       struct cli_state *cli,
		       const char *nt_path,
		       const char *unix_path);
NTSTATUS gpo_sync_directories(TALLOC_CTX *mem_ctx,
			      struct cli_state *cli,
			      const char *nt_path,
			      const char *local_path);

/* The following definitions come from libgpo/gpo_ini.c  */

NTSTATUS parse_gpt_ini(TALLOC_CTX *mem_ctx,
		       const char *filename,
		       uint32_t *version,
		       char **display_name);

/* The following definitions come from libgpo/gpo_reg.c  */

struct nt_user_token *registry_create_system_token(TALLOC_CTX *mem_ctx);
WERROR gp_init_reg_ctx(TALLOC_CTX *mem_ctx,
		       const char *initial_path,
		       uint32_t desired_access,
		       const struct nt_user_token *token,
		       struct gp_registry_context **reg_ctx);
void gp_free_reg_ctx(struct gp_registry_context *reg_ctx);
WERROR gp_store_reg_subkey(TALLOC_CTX *mem_ctx,
			   const char *subkeyname,
			   struct registry_key *curr_key,
			   struct registry_key **new_key);
WERROR gp_read_reg_subkey(TALLOC_CTX *mem_ctx,
			  struct gp_registry_context *reg_ctx,
			  const char *subkeyname,
			  struct registry_key **key);
WERROR gp_store_reg_val_sz(TALLOC_CTX *mem_ctx,
			   struct registry_key *key,
			   const char *val_name,
			   const char *val);
WERROR gp_read_reg_val_sz(TALLOC_CTX *mem_ctx,
			  struct registry_key *key,
			  const char *val_name,
			  const char **val);
WERROR gp_reg_state_store(TALLOC_CTX *mem_ctx,
			  uint32_t flags,
			  const char *dn,
			  const struct nt_user_token *token,
			  struct GROUP_POLICY_OBJECT *gpo_list);
WERROR gp_reg_state_read(TALLOC_CTX *mem_ctx,
			 uint32_t flags,
			 const struct dom_sid *sid,
			 struct GROUP_POLICY_OBJECT **gpo_list);
WERROR gp_secure_key(TALLOC_CTX *mem_ctx,
		     uint32_t flags,
		     struct registry_key *key,
		     const struct dom_sid *sid);
void dump_reg_val(int lvl, const char *direction,
		  const char *key, const char *subkey,
		  struct registry_value *val);
void dump_reg_entry(uint32_t flags,
		    const char *dir,
		    struct gp_registry_entry *entry);
void dump_reg_entries(uint32_t flags,
		      const char *dir,
		      struct gp_registry_entry *entries,
		      size_t num_entries);
bool add_gp_registry_entry_to_array(TALLOC_CTX *mem_ctx,
				    struct gp_registry_entry *entry,
				    struct gp_registry_entry **entries,
				    size_t *num);
WERROR reg_apply_registry_entry(TALLOC_CTX *mem_ctx,
				struct registry_key *root_key,
				struct gp_registry_context *reg_ctx,
				struct gp_registry_entry *entry,
				const struct nt_user_token *token,
				uint32_t flags);